#include "arbitrage_strategy_matcher.hpp"

#include "exchange/exchange_client.h"
#include "exchange/exchange_listener.h"
#include "consumer/consumer.h"
#include "model/options.h"
#include "strategy/trading_strategy.h"

#include <boost/log/trivial.hpp>

#include <chrono>
#include <future>
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <utility>


struct ArbitrageStrategyOptions {
  std::unordered_map<std::string, ExchangeParams> m_exchange_params;
  double m_default_amount;
  double m_min_amount;
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
        if (m_opts.m_default_amount < 0.0 || m_opts.m_default_amount > 0.1) {
          throw std::invalid_argument("Invalid order amount, please check configuration or change hardcoded limits");
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
      auto res = p.second->GetAccountBalance();
      if (!res) {
        throw std::runtime_error("Failed getting account balance for " + p.first);
      }
      m_balances.insert(std::make_pair(p.first, std::move(res.Get())));

      // FIXME: support multiple pairs ?
      auto res_orders = p.second->GetOpenOrders(SymbolPairId::BTC_USDT);
      if (!res_orders) {
        throw std::runtime_error("Failed getting open orders for " + p.first);
      }
      m_open_orders.insert(std::make_pair(p.first, std::move(res_orders.Get())));
    }
    m_is_up_to_date = true;
  }

  virtual void OnTicker(const Ticker& ticker) override {
    std::unique_lock<std::mutex> scoped_lock(m_mutex);
    //std::cout << ticker << std::endl;
    SymbolPair current_symbol_pair = ticker.m_symbol;
    SymbolPairId current_symbol_id = SymbolPairId(current_symbol_pair);
    m_tickers[current_symbol_id][ticker.m_exchange] = ticker;
    auto match_opt = m_matcher.FindMatch(m_tickers[current_symbol_id]);
    if (match_opt.has_value()) {
      auto match = match_opt.value();
      const auto& best_bid_ticker = match.m_best_bid;
      const auto& best_ask_ticker = match.m_best_ask;
      const auto& best_bid_exchange = best_bid_ticker.m_exchange;
      const auto& best_ask_exchange = best_ask_ticker.m_exchange;
      // Limit trades rate
      using namespace std::chrono;
      auto now_us = duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
      if (now_us - m_last_trade_us < m_opts.m_min_trade_interval_us) {
        BOOST_LOG_TRIVIAL(info) << "Rate limiting trades";
        return;
      }
      // Check tickers arrival timestamp
      auto best_bid_ticker_age = now_us - best_bid_ticker.m_arrived_ts;
      auto best_ask_ticker_age = now_us - best_ask_ticker.m_arrived_ts;
      BOOST_LOG_TRIVIAL(info) << "Best bid ticker age (" << best_bid_exchange
            << "): " << std::to_string(best_bid_ticker_age) << ", best ask ticker age (" << best_ask_exchange
            << "): " << std::to_string(best_ask_ticker_age);
      if (best_bid_ticker_age > m_opts.m_max_ticker_age_us
        || best_ask_ticker_age > m_opts.m_max_ticker_age_us) {
        BOOST_LOG_TRIVIAL(warning) << "Ticker is too old";
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
      if (m_account_managers.count(best_bid_exchange) > 0
        && m_account_managers.count(best_ask_exchange) > 0) {
        // Set trade amount to minimum across best bid, best ask and default amount
        double vol = m_opts.m_default_amount;
        if (best_bid_ticker.m_bid_vol.has_value()) {
          vol = std::min(best_bid_ticker.m_bid_vol.value(), vol);
        }
        if (best_ask_ticker.m_ask_vol.has_value()) {
          vol = std::min(best_ask_ticker.m_ask_vol.value(), vol);
        }
        // Make sure the amount is above minimum
        if (vol < m_opts.m_min_amount) {
          BOOST_LOG_TRIVIAL(warning) << "Order amount below minimum.";
          return;
        }
        // Finally
        // if (!CanSell(best_bid_exchange, current_symbol_pair, vol) || !CanBuy(best_ask_exchange, current_symbol_pair, vol, best_ask_ticker.m_ask)) {
        //   BOOST_LOG_TRIVIAL(warning) << "Not enough funds.";
        //   UpdateBalances(best_bid_exchange, best_ask_exchange);
        //   return;
        // }
        // if (HasOpenOrders(best_bid_exchange) || HasOpenOrders(best_ask_exchange)) {
        //   BOOST_LOG_TRIVIAL(warning) << "There are open orders.";
        //   UpdateOpenOrders(current_symbol_id, best_bid_exchange, best_ask_exchange);
        //   return;
        // }
        auto f1 = std::async(std::launch::async, &ExchangeClient::LimitOrder, m_account_managers[best_bid_exchange].get(),
            current_symbol_id, Side::SELL, vol, best_bid_ticker.m_bid);
        auto f2 = std::async(std::launch::async, &ExchangeClient::LimitOrder, m_account_managers[best_ask_exchange].get(),
            current_symbol_id, Side::BUY, vol, best_ask_ticker.m_ask);
        m_last_trade_us = now_us;
        auto f1_res = f1.get();
        if (!f1_res) {
          BOOST_LOG_TRIVIAL(warning) << "Error sending order for " << best_bid_exchange << ": " << f1_res.GetErrorMsg();;
        }
        auto f2_res = f2.get();
        if (!f2_res) {
          BOOST_LOG_TRIVIAL(warning) << "Error sending order for " << best_ask_exchange << ": " << f2_res.GetErrorMsg();
        }
        BOOST_LOG_TRIVIAL(info) << "Arbitrage match good enough. Order sent!";
        BOOST_LOG_TRIVIAL(info) << match << std::endl;
        UpdateBalances(best_bid_exchange, best_ask_exchange);
        UpdateOpenOrders(current_symbol_id, best_bid_exchange, best_ask_exchange);
      }
    }
  }

  virtual void OnOrderBookUpdate(const OrderBook& order_book) {
    const auto& best_ask = order_book.GetBestAsk();
    const auto& best_bid = order_book.GetBestBid();
    Ticker ticker;
    // TODO: get precision form order book object
    ticker.m_ask = double(best_ask.GetPrice())/100000.0;
    ticker.m_ask_vol = std::optional<double>(best_ask.GetVolume());
    ticker.m_bid = double(best_bid.GetPrice())/100000.0;
    ticker.m_bid_vol = std::optional<double>(best_bid.GetVolume());
    ticker.m_source_ts = std::optional<int64_t>(std::min(best_ask.GetTimestamp(), best_bid.GetTimestamp()));
    using namespace std::chrono;
    auto now = system_clock::now();
    system_clock::duration tp = now.time_since_epoch();
    microseconds us = duration_cast<microseconds>(tp);
    ticker.m_arrived_ts = us.count();
    ticker.m_exchange = order_book.GetExchangeName();
    // TODO: hardcoded
    ticker.m_symbol = order_book.GetSymbolName();
    //BOOST_LOG_TRIVIAL(info) << "Arrived: " << ticker.m_arrived_ts << ", best ask: " << best_ask.GetTimestamp() << ", best bid: " << best_bid.GetTimestamp() << std::endl;
    //BOOST_LOG_TRIVIAL(info) << ticker << std::endl;
    OnTicker(ticker);
  }

  virtual void OnConnectionClose(const std::string& name) override {
    std::unique_lock<std::mutex> scoped_lock(m_mutex);
    for (auto p : m_tickers) {
      p.second.erase(name);
    }
  }

private:

  inline bool CanSell(const std::string& exchange_name, SymbolPair symbol_pair, double amount) {
    BOOST_LOG_TRIVIAL(debug) << "CanSell";
    assert(m_balances.count(exchange_name) > 0);
    double balance = m_balances.at(exchange_name).GetBalance(symbol_pair.GetBaseAsset());
    double min_balance = amount * (1.0 + m_opts.m_exchange_params.at(exchange_name).m_fee);
    BOOST_LOG_TRIVIAL(debug) << "Minimum required BTC balance on " << exchange_name << " is " << std::to_string(min_balance);
    return balance > min_balance;
  }

  inline bool CanBuy(const std::string& exchange_name, SymbolPair symbol_pair, double amount, double price) {
    BOOST_LOG_TRIVIAL(debug) << "CanBuy";
    assert(m_balances.count(exchange_name) > 0);
    double balance = m_balances.at(exchange_name).GetBalance(symbol_pair.GetQuoteAsset());
    double min_balance = amount * price * (1.0 + m_opts.m_exchange_params.at(exchange_name).m_fee);
    BOOST_LOG_TRIVIAL(debug) << "Minimum required USDT balance on " << exchange_name << " is " << std::to_string(min_balance);
    return balance > min_balance;
  }

  inline bool HasOpenOrders(const std::string& exchange_name) {
    if (m_open_orders.count(exchange_name) > 0) {
      const auto& orders = m_open_orders.at(exchange_name);
      if (orders.size() > 0) {
        return true;
      }
      return false;
    }
    // No information, so better to assume there are open orders
    return true;
  }

  bool UpdateBalances(const std::string& best_bid_exchange, const std::string& best_ask_exchange) {
    auto acc_f1 = std::async(std::launch::async, &ExchangeClient::GetAccountBalance, m_account_managers[best_bid_exchange].get());
    auto acc_f2 = std::async(std::launch::async, &ExchangeClient::GetAccountBalance, m_account_managers[best_ask_exchange].get());
    auto res_best_bid = acc_f1.get();
    auto res_best_ask = acc_f2.get();
    if (!res_best_bid) {
      BOOST_LOG_TRIVIAL(warning) << "Could not get account balance for " << best_bid_exchange;
      return false;
    }
    m_balances.insert_or_assign(best_bid_exchange, std::move(res_best_bid.Get()));
    if (!res_best_ask) {
      BOOST_LOG_TRIVIAL(warning) << "Could not get account balance for " << best_ask_exchange;
      return false;
    }
    m_balances.insert_or_assign(best_ask_exchange, std::move(res_best_ask.Get()));
    return true;
  }

  // TODO: FIXME: here we currently update for one pair and overwirite with another in case of arbitrage for multiple pairs
  bool UpdateOpenOrders(SymbolPairId symbol_pair, const std::string& best_bid_exchange, const std::string& best_ask_exchange) {
    auto acc_f1 = std::async(std::launch::async, &ExchangeClient::GetOpenOrders, m_account_managers[best_bid_exchange].get(), symbol_pair);
    auto acc_f2 = std::async(std::launch::async, &ExchangeClient::GetOpenOrders, m_account_managers[best_ask_exchange].get(), symbol_pair);
    auto res_best_bid = acc_f1.get();
    auto res_best_ask = acc_f2.get();
    if (!res_best_bid) {
      BOOST_LOG_TRIVIAL(warning) << "Could not get open orders for " << best_bid_exchange;
      return false;
    }
    m_open_orders.insert_or_assign(best_bid_exchange, std::move(res_best_bid.Get()));
    if (!res_best_ask) {
      BOOST_LOG_TRIVIAL(warning) << "Could not get open orders for " << best_ask_exchange;
      return false;
    }
    m_open_orders.insert_or_assign(best_ask_exchange, std::move(res_best_ask.Get()));
    return true;
  }
private:
  std::mutex m_mutex;
  ArbitrageStrategyOptions m_opts;
  ArbitrageStrategyMatcher m_matcher;
  // symbol -> (exchange -> ticker)
  std::map<SymbolPairId, std::map<std::string, Ticker>> m_tickers;
  std::unordered_map<std::string, AccountBalance> m_balances;
  std::unordered_map<std::string, std::vector<Order>> m_open_orders;
  int64_t m_last_trade_us;
  bool m_is_up_to_date;
};