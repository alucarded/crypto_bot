#include "arbitrage_strategy_matcher.hpp"

#include "exchange/exchange_client.h"
#include "exchange/exchange_listener.h"
#include "consumer/consumer.h"
#include "model/options.h"
#include "strategy/trading_strategy.h"
#include "utils/math.hpp"
#include "utils/spinlock.hpp"

#include <boost/log/trivial.hpp>

#include <atomic>
#include <chrono>
#include <future>
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <utility>

using namespace std::chrono;

struct ArbitrageStrategyOptions {
  std::unordered_map<std::string, ExchangeParams> m_exchange_params;
  std::unordered_map<SymbolId, double> m_default_amount;
  std::unordered_map<SymbolId, double> m_min_amount;
  int64_t m_max_ticker_age_us;
  int64_t m_max_ticker_delay_us;
  int64_t m_min_trade_interval_us;
  double m_base_currency_ratio;
  double m_allowed_deviation;
};

class ArbitrageStrategy : public TradingStrategy, public ExchangeListener {
public:
    ArbitrageStrategy(const ArbitrageStrategyOptions& opts)
      : m_opts(opts), m_matcher(opts.m_exchange_params), m_last_trade_us(0) {
        // Validations
        if (m_opts.m_exchange_params.size() < 2) {
          throw std::invalid_argument("Parameters for at least 2 exchanges required");
        }
        for (const auto& p : m_opts.m_default_amount) {
          if (p.second < 0.0) {
            throw std::invalid_argument("Invalid order amount, please check configuration or change hardcoded limits");
          }
        }
  }

  // For testing
  ArbitrageStrategy(const ArbitrageStrategyOptions& opts, const ArbitrageStrategyMatcher& matcher)
    : m_opts(opts), m_matcher(matcher), m_last_trade_us(0) {
  }

  // For testing
  ArbitrageStrategy() : m_last_trade_us(0) {

  }

  void Initialize() {
    for (const auto& p : m_account_managers) {
      p.second->Initialize();
    }
  }

  virtual void OnTicker(const Ticker& ticker) override {
    //std::cout << ticker << std::endl;
    SymbolPair current_symbol_pair = ticker.m_symbol;
    SymbolPairId current_symbol_id = SymbolPairId(current_symbol_pair);
    m_tickers_lock.lock();
    m_tickers[current_symbol_id][ticker.m_exchange] = ticker;
    auto match_opt = m_matcher.FindMatch(m_tickers[current_symbol_id]);
    m_tickers_lock.unlock();
    if (match_opt.has_value()) {
      auto match = match_opt.value();
      const auto& best_bid_ticker = match.m_best_bid;
      const auto& best_ask_ticker = match.m_best_ask;
      const auto& best_bid_exchange = best_bid_ticker.m_exchange;
      const auto& best_ask_exchange = best_ask_ticker.m_exchange;
      // Check tickers arrival timestamp
      auto now_us = duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
      auto best_bid_ticker_age = now_us - best_bid_ticker.m_arrived_ts;
      auto best_ask_ticker_age = now_us - best_ask_ticker.m_arrived_ts;
      BOOST_LOG_TRIVIAL(debug) << "Best bid ticker age (" << best_bid_exchange
            << " - " << best_bid_ticker.m_symbol << "): " << std::to_string(best_bid_ticker_age)
            << ", best ask ticker age (" << best_ask_exchange
            << " - " << best_ask_ticker.m_symbol << "): " << std::to_string(best_ask_ticker_age);
      if (best_bid_ticker_age > m_opts.m_max_ticker_age_us) {
        BOOST_LOG_TRIVIAL(debug) << "Ticker " << best_bid_ticker.m_exchange << " " << best_bid_ticker.m_symbol << " is too old";
        return;
      }
      if (best_ask_ticker_age > m_opts.m_max_ticker_age_us) {
        BOOST_LOG_TRIVIAL(debug) << "Ticker " << best_ask_ticker.m_exchange << " " << best_ask_ticker.m_symbol << " is too old";
        return;
      }
      // Check tickers source timestamp, if present
      auto best_bid_ticker_delay = best_bid_ticker.m_source_ts.has_value()
          ? best_bid_ticker.m_arrived_ts - best_bid_ticker.m_source_ts.value() : -1;
      auto best_ask_ticker_delay = best_ask_ticker.m_source_ts.has_value()
          ? best_ask_ticker.m_arrived_ts - best_ask_ticker.m_source_ts.value() : -1;
      BOOST_LOG_TRIVIAL(info) << "Best bid ticker delay (" << best_bid_exchange
            << "): " << std::to_string(best_bid_ticker_delay) << ", best ask ticker delay (" << best_ask_exchange
            << "): " << std::to_string(best_ask_ticker_delay);
      if (best_bid_ticker_delay > m_opts.m_max_ticker_delay_us
          || best_ask_ticker_delay > m_opts.m_max_ticker_delay_us) {
        BOOST_LOG_TRIVIAL(warning) << "Ticker arrived with too big delay";
        return;
      }
      // std::cout << "Got match between " << best_bid_exchange << " and " << best_ask_exchange
      //   << " with profit " << std::to_string(match.m_profit) << std::endl;
      auto best_bid_count = m_account_managers.count(best_bid_exchange);
      auto best_ask_count = m_account_managers.count(best_ask_exchange);
      BOOST_LOG_TRIVIAL(debug) << "Got account manager for best bid (" << best_bid_exchange << "): " << best_bid_count;
      BOOST_LOG_TRIVIAL(debug) << "Got account manager for best ask (" << best_ask_exchange << "): " << best_ask_count;
      if (best_bid_count > 0 && best_ask_count > 0) {
        // Set trade amount to minimum across best bid, best ask and default amount
        double vol = m_opts.m_default_amount[current_symbol_pair.GetBaseAsset()];
        if (best_bid_ticker.m_bid_vol.has_value()) {
          vol = std::min(best_bid_ticker.m_bid_vol.value(), vol);
        }
        if (best_ask_ticker.m_ask_vol.has_value()) {
          vol = std::min(best_ask_ticker.m_ask_vol.value(), vol);
        }
        // Make sure the amount is above minimum
        if (vol < m_opts.m_min_amount[current_symbol_pair.GetBaseAsset()]) {
          BOOST_LOG_TRIVIAL(warning) << "Order amount below minimum.";
          return;
        }
        // TODO: check available amount and use to set order amount
        if (!CanSell(best_bid_exchange, current_symbol_pair, vol) || !CanBuy(best_ask_exchange, current_symbol_pair, vol, best_ask_ticker.m_ask)) {
          BOOST_LOG_TRIVIAL(warning) << "Not enough funds.";
          return;
        }
        // if (HasOpenOrders(best_bid_exchange) || HasOpenOrders(best_ask_exchange)) {
        //   BOOST_LOG_TRIVIAL(warning) << "There are open orders.";
        //   return;
        // }
        if (best_bid_ticker.m_bid == best_bid_ticker.m_ask) {
          BOOST_LOG_TRIVIAL(warning) << "Best bid ticker has same ask!";
          return;
        }
        if (best_ask_ticker.m_ask == best_ask_ticker.m_bid) {
          BOOST_LOG_TRIVIAL(warning) << "Best ask ticker has same bid!";
          return;
        }
        // Limit trades rate
        auto now_us = duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
        int64_t last_trade_snap = m_last_trade_us.exchange(now_us);
        if (now_us - last_trade_snap < m_opts.m_min_trade_interval_us) {
          BOOST_LOG_TRIVIAL(info) << "Rate limiting trades";
          return;
        }
        // Only one thread can send orders at any given point,
        // skip if another thread is currently sending orders.
        BOOST_LOG_TRIVIAL(debug) << "Trying to lock";
        std::unique_lock<std::mutex> lock_obj(m_orders_mutex, std::defer_lock);
        if (lock_obj.try_lock()) {
          auto f1 = std::async(std::launch::async, &ExchangeClient::LimitOrder, m_account_managers[best_bid_exchange].get(),
              current_symbol_id, Side::SELL, vol, best_bid_ticker.m_bid);
          auto f2 = std::async(std::launch::async, &ExchangeClient::LimitOrder, m_account_managers[best_ask_exchange].get(),
              current_symbol_id, Side::BUY, vol, best_ask_ticker.m_ask);
          auto f1_res = f1.get();
          if (!f1_res) {
            BOOST_LOG_TRIVIAL(warning) << "Error sending order for " << best_bid_exchange << ": " << f1_res.GetErrorMsg();
            std::exit(1);
          }
          auto f2_res = f2.get();
          if (!f2_res) {
            BOOST_LOG_TRIVIAL(warning) << "Error sending order for " << best_ask_exchange << ": " << f2_res.GetErrorMsg();
            std::exit(1);
          }
          BOOST_LOG_TRIVIAL(info) << "Arbitrage match good enough. Order sent!";
          BOOST_LOG_TRIVIAL(info) << match << std::endl;
          lock_obj.unlock();
        }
      }
    }
  }

  virtual void OnOrderBookUpdate(const OrderBook& order_book) {
    const auto& best_ask = order_book.GetBestAsk();
    const auto& best_bid = order_book.GetBestBid();
    const auto& precision_settings = order_book.GetPrecisionSettings();
    Ticker ticker;
    ticker.m_ask = double(best_ask.GetPrice())/quick_pow10(precision_settings.m_price_precision);
    ticker.m_ask_vol = std::optional<double>(best_ask.GetVolume());
    ticker.m_bid = double(best_bid.GetPrice())/quick_pow10(precision_settings.m_price_precision);
    ticker.m_bid_vol = std::optional<double>(best_bid.GetVolume());
    ticker.m_source_ts = std::optional<int64_t>(std::min(best_ask.GetTimestamp(), best_bid.GetTimestamp()));
    using namespace std::chrono;
    auto now = system_clock::now();
    system_clock::duration tp = now.time_since_epoch();
    microseconds us = duration_cast<microseconds>(tp);
    ticker.m_arrived_ts = us.count();
    ticker.m_exchange = order_book.GetExchangeName();
    // TODO: hardcoded
    ticker.m_symbol = order_book.GetSymbolPairId();
    //BOOST_LOG_TRIVIAL(info) << "Arrived: " << ticker.m_arrived_ts << ", best ask: " << best_ask.GetTimestamp() << ", best bid: " << best_bid.GetTimestamp() << std::endl;
    //BOOST_LOG_TRIVIAL(info) << ticker << std::endl;
    OnTicker(ticker);
  }

  virtual void OnConnectionOpen(const std::string& name) override {
    m_orders_mutex.unlock();
  }

  virtual void OnConnectionClose(const std::string& name) override {
    m_tickers_lock.lock();
    for (auto p : m_tickers) {
      p.second.erase(name);
    }
    m_tickers_lock.unlock();
  }

private:

  inline bool CanSell(const std::string& exchange_name, SymbolPair symbol_pair, double amount) {
    SymbolId symbol_id = symbol_pair.GetBaseAsset();
    double balance = m_account_managers[exchange_name]->GetFreeBalance(symbol_id);
    return balance > amount;
  }

  inline bool CanBuy(const std::string& exchange_name, SymbolPair symbol_pair, double amount, double price) {
    SymbolId symbol_id = symbol_pair.GetQuoteAsset();
    double balance = m_account_managers[exchange_name]->GetFreeBalance(symbol_id);
    return balance > price * amount;
  }

  inline bool HasOpenOrders(const std::string& exchange_name) {
    return m_account_managers[exchange_name]->HasOpenOrders();
  }

private:
  ArbitrageStrategyOptions m_opts;
  ArbitrageStrategyMatcher m_matcher;
  // symbol -> (exchange -> ticker)
  std::map<SymbolPairId, std::map<std::string, Ticker>> m_tickers;
  std::mutex m_orders_mutex;
  cryptobot::spinlock m_tickers_lock;
  std::atomic<int64_t> m_last_trade_us;
};