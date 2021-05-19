#include "arbitrage_order_calculator.h"
#include "arbitrage_strategy_matcher.hpp"
#include "arbitrage_strategy_options.h"

#include "exchange/exchange_client.h"
#include "exchange/exchange_listener.h"
#include "consumer/consumer.h"
#include "model/options.h"
#include "strategy/mean_reversion_signal.hpp"
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

  virtual void OnBookTicker(const Ticker& ticker) override {
    SymbolPair current_symbol_pair = ticker.m_symbol;
    SymbolPairId current_symbol_id = SymbolPairId(current_symbol_pair);
#ifdef WITH_MEAN_REVERSION_SIGNAL
    // Only one exchange guarantees the block will be run by only one thread
    if (ticker.m_exchange == "binance") {
      // Update signal
      if (m_mrs.find(current_symbol_id) == m_mrs.end()) {
        m_mrs.emplace(current_symbol_id, MeanReversionSignal(8u, 21u, 55u, 8u));
      }
      m_mrs[current_symbol_id].Consume(ticker);
    }
#endif
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
      auto now_us = m_opts.time_provider_fcn(ticker);
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
      BOOST_LOG_TRIVIAL(trace) << "Got account manager for best bid (" << best_bid_exchange << "): " << best_bid_count;
      BOOST_LOG_TRIVIAL(trace) << "Got account manager for best ask (" << best_ask_exchange << "): " << best_ask_count;
      if (best_bid_count > 0 && best_ask_count > 0) {
        double order_size = CalculateOrderSize(best_bid_ticker, best_ask_ticker);
        // Make sure the amount is above minimum
        if (order_size < m_opts.m_min_amount[current_symbol_pair.GetBaseAsset()]) {
          BOOST_LOG_TRIVIAL(warning) << "Order amount below minimum.";
          return;
        }

        if (!m_account_managers[best_bid_exchange]->IsAccountSynced() || !m_account_managers[best_ask_exchange]->IsAccountSynced()) {
          BOOST_LOG_TRIVIAL(warning) << "Accounts not synced.";
          return;
        }

        if (HasOpenOrders(current_symbol_id)) {
          BOOST_LOG_TRIVIAL(warning) << "Not doing basic arbitrage since there are already open orders for " << current_symbol_id;
          return;
        }

        std::unique_lock<std::mutex> lock_obj(m_orders_mutex, std::defer_lock);
        // Only one thread can send orders at any given point,
        // skip if another thread is currently sending orders.
        if (lock_obj.try_lock()) {
#ifdef WITH_MEAN_REVERSION_SIGNAL
          Prediction prediction = m_mrs[current_symbol_id].Predict(best_bid_ticker.m_bid, best_ask_ticker.m_ask);
          switch(prediction.price_outlook) {
            case PriceOutlook::BEARISH: {
                // Use limit order tilted by fee
                double fee = m_opts.m_exchange_params.at(best_bid_exchange).m_fee;
                auto f1 = std::async(std::launch::async, &ExchangeClient::LimitOrder, m_account_managers[best_bid_exchange].get(),
                    current_symbol_id, Side::SELL, order_size, (1.0 - fee) * best_bid_ticker.m_bid);
                auto f1_res = f1.get();
                if (!f1_res) {
                  BOOST_LOG_TRIVIAL(warning) << "Error sending order for " << best_bid_exchange << ": " << f1_res.GetErrorMsg();
                  std::exit(1);
                }
                auto f2 = std::async(std::launch::async, &ExchangeClient::LimitOrder, m_account_managers[best_ask_exchange].get(),
                    current_symbol_id, Side::BUY, order_size, prediction.target_price);
                auto f2_res = f2.get();
                if (!f2_res) {
                  BOOST_LOG_TRIVIAL(warning) << "Error sending order for " << best_ask_exchange << ": " << f2_res.GetErrorMsg();
                  std::exit(1);
                }
              }
              break;
            case PriceOutlook::NEUTRAL: {
#endif
                SendBasicArbitrageOrders(best_bid_ticker, best_ask_ticker, order_size);
#ifdef WITH_MEAN_REVERSION_SIGNAL
              }
              break;
            case PriceOutlook::BULLISH: {
                // Use limit order tilted by fee
                double fee = m_opts.m_exchange_params.at(best_ask_exchange).m_fee;
                auto f1 = std::async(std::launch::async, &ExchangeClient::LimitOrder, m_account_managers[best_ask_exchange].get(),
                    current_symbol_id, Side::BUY, order_size, (1.0 + fee) * best_ask_ticker.m_ask);
                auto f1_res = f1.get();
                if (!f1_res) {
                  BOOST_LOG_TRIVIAL(warning) << "Error sending order for " << best_bid_exchange << ": " << f1_res.GetErrorMsg();
                  std::exit(1);
                }
                auto f2 = std::async(std::launch::async, &ExchangeClient::LimitOrder, m_account_managers[best_bid_exchange].get(),
                    current_symbol_id, Side::SELL, order_size, prediction.target_price);
                auto f2_res = f2.get();
                if (!f2_res) {
                  BOOST_LOG_TRIVIAL(warning) << "Error sending order for " << best_ask_exchange << ": " << f2_res.GetErrorMsg();
                  std::exit(1);
                }
              }
              break;
            default:
              throw std::runtime_error("Unsupported PriceOutlook value");
          };
#endif
          BOOST_LOG_TRIVIAL(info) << "Arbitrage match. Orders sent!";
          BOOST_LOG_TRIVIAL(info) << "Best bid ticker: " << best_bid_ticker;
          BOOST_LOG_TRIVIAL(info) << "Best ask ticker: " << best_ask_ticker;
          lock_obj.unlock();
        }
      }
    }
  }

  virtual void OnOrderBookUpdate(const OrderBook& order_book) {
    OnBookTicker(ExchangeListener::TickerFromOrderBook(order_book));
  }

  virtual void OnConnectionOpen(const std::string& name) override {
  }

  virtual void OnConnectionClose(const std::string& name) override {
    m_tickers_lock.lock();
    for (auto p : m_tickers) {
      p.second.erase(name);
    }
    m_tickers_lock.unlock();
  }

private:

  void SendBasicArbitrageOrders(const Ticker& best_bid_ticker, const Ticker& best_ask_ticker, double vol) {
    const auto& best_bid_exchange = best_bid_ticker.m_exchange;
    const auto& best_ask_exchange = best_ask_ticker.m_exchange;
    SymbolPairId current_symbol_id = SymbolPairId(best_bid_ticker.m_symbol); 
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
  }

  inline double GetBaseBalance(const std::string& exchange_name, SymbolPair symbol_pair) const {
    SymbolId symbol_id = symbol_pair.GetBaseAsset();
    return m_account_managers.at(exchange_name)->GetFreeBalance(symbol_id);
  }

  inline double GetQuoteBalance(const std::string& exchange_name, SymbolPair symbol_pair) const {
    SymbolId symbol_id = symbol_pair.GetQuoteAsset();
    return m_account_managers.at(exchange_name)->GetFreeBalance(symbol_id);
  }

  double CalculateOrderSize(const Ticker& best_bid_ticker, const Ticker& best_ask_ticker) const {
    SymbolPair sp = best_bid_ticker.m_symbol;
    // Find maximum volume for arbitrage
    // Set constraints on traded amount based on book volumes
    assert(best_bid_ticker.m_bid_vol.has_value());
    assert(best_ask_ticker.m_ask_vol.has_value());
    double book_bid_vol = best_bid_ticker.m_bid_vol.value();
    double book_ask_vol = best_ask_ticker.m_ask_vol.value();
    // 1. Allow maximum 20% of smaller amount from the books, but not less than default amount (if possible)
    // This is to decrease risk of not being filled immedaitely (or almost immediately)
    double base_vol = std::min(book_bid_vol, book_ask_vol);
    BOOST_LOG_TRIVIAL(debug) << "Available for arbitrage: " << base_vol << " " << sp;
    double default_vol = m_opts.m_default_amount.at(sp.GetBaseAsset());
    if (base_vol > default_vol) {
      base_vol = std::max(0.2 * base_vol, default_vol);
    }

    double base_asset_balance = GetBaseBalance(best_bid_ticker.m_exchange, sp);
    double quote_asset_balance = GetQuoteBalance(best_ask_ticker.m_exchange, sp);
    BOOST_LOG_TRIVIAL(debug) << best_bid_ticker.m_exchange << " - base asset account balance: " << base_asset_balance;
    BOOST_LOG_TRIVIAL(debug) << best_ask_ticker.m_exchange << " - quote asset account balance: " << quote_asset_balance;
    // 2. Allow maximum 90% of actual balance to be traded
    // This is to decrease risk of order failure due to insufficient funds (workaround inaccurate balance or not taking fee into account)
    double tradable_base_balance = 0.90 * base_asset_balance;
    double tradable_quote_balance = 0.90 * quote_asset_balance;
    // Check with available balances
    double quote_vol = std::min(base_vol * best_ask_ticker.m_ask, tradable_quote_balance);
    // Maximum volume available for arbitrage
    base_vol = std::min({base_vol, quote_vol/best_ask_ticker.m_ask, tradable_base_balance});
    BOOST_LOG_TRIVIAL(debug) << "tradable_base_balance=" << tradable_base_balance << ", tradable_quote_balance=" << tradable_quote_balance
        << ", quote_vol=" << quote_vol << ", base_vol=" << base_vol;

    return base_vol;
  }

  bool HasOpenOrders(SymbolPairId pair_id) const {
    for (const auto& p : m_account_managers) {
      const auto& manager_ptr = p.second;
      if (manager_ptr->HasOpenOrders(pair_id)) {
        return true;
      }
    }
    return false;
  }
private:
  ArbitrageStrategyOptions m_opts;
  ArbitrageStrategyMatcher m_matcher;
  // symbol -> (exchange -> ticker)
  std::map<SymbolPairId, std::map<std::string, Ticker>> m_tickers;
  std::map<SymbolPairId, MeanReversionSignal> m_mrs;
  std::mutex m_orders_mutex;
  cryptobot::spinlock m_tickers_lock;
  std::atomic<int64_t> m_last_trade_us;
};