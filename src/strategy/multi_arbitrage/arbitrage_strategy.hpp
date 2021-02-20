#include "arbitrage_strategy_matcher.hpp"

#include "exchange/exchange_client.h"
#include "consumer/consumer.h"
#include "options.h"
#include "strategy/trading_strategy.h"
#include "ticker_subscriber.h"

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
};

class ArbitrageStrategy : public TradingStrategy, public TickerSubscriber {
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
      m_balances.insert(std::make_pair(p.first, std::move(p.second->GetAccountBalance())));
    }
  }

  virtual void execute(const std::string& updated_ticker, const std::map<std::string, Ticker>& tickers) override {
    // unused
  }

  virtual void OnTicker(const Ticker& ticker) override {
    std::unique_lock<std::mutex> scoped_lock(m_mutex);
    //std::cout << "Ticker." << std::endl << "Bid: " << std::to_string(ticker.m_bid) << std::endl << "Ask: " << std::to_string(ticker.m_ask) << std::endl;
    m_tickers[ticker.m_exchange] = ticker;
    auto match_opt = m_matcher.FindMatch(m_tickers);
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
        if (!CanSell(best_bid_exchange, vol) || !CanBuy(best_ask_exchange, vol, best_ask_ticker.m_ask)) {
          BOOST_LOG_TRIVIAL(warning) << "Not enough funds.";
          return;
        }
        auto f1 = std::async(std::launch::async, &ExchangeClient::LimitOrder, m_account_managers[best_bid_exchange].get(),
            "BTCUSDT", Side::ASK, vol, best_bid_ticker.m_bid);
        auto f2 = std::async(std::launch::async, &ExchangeClient::LimitOrder, m_account_managers[best_ask_exchange].get(),
            "BTCUSDT", Side::BID, vol, best_ask_ticker.m_ask);
        m_last_trade_us = now_us;
        auto f1_res = f1.get();
        auto f2_res = f2.get();
        BOOST_LOG_TRIVIAL(info) << match << std::endl << "Response from " << best_bid_exchange << ": " << std::endl << f1_res << std::endl
            << "Response from " << best_ask_exchange << ": " << std::endl << f2_res << std::endl;
        UpdateBalances(best_bid_exchange, best_ask_exchange);
      }
    }
  }

  virtual void OnDisconnected(const std::string& exchange_name) override {
    std::unique_lock<std::mutex> scoped_lock(m_mutex);
    m_tickers.erase(exchange_name);
  }

private:

  inline bool CanSell(const std::string& exchange_name, double amount) {
    assert(m_balances.count(exchange_name) > 0);
    // TODO: here we assume BTCUSDT
    double balance = m_balances.at(exchange_name).GetBalance("BTC");
    double min_balance = amount * (1.0 + m_opts.m_exchange_params.at(exchange_name).m_fee);
    BOOST_LOG_TRIVIAL(debug) << "Minimum required BTC balance on " << exchange_name << " is " << std::to_string(min_balance);
    return balance > min_balance;
  }

  inline bool CanBuy(const std::string& exchange_name, double amount, double price) {
    assert(m_balances.count(exchange_name) > 0);
    double balance = m_balances.at(exchange_name).GetBalance("USDT");
    double min_balance = amount * price * (1.0 + m_opts.m_exchange_params.at(exchange_name).m_fee);
    BOOST_LOG_TRIVIAL(debug) << "Minimum required USDT balance on " << exchange_name << " is " << std::to_string(min_balance);
    return balance > min_balance;
  }

  void UpdateBalances(const std::string& best_bid_exchange, const std::string& best_ask_exchange) {
    auto acc_f1 = std::async(std::launch::async, &ExchangeClient::GetAccountBalance, m_account_managers[best_bid_exchange].get());
    auto acc_f2 = std::async(std::launch::async, &ExchangeClient::GetAccountBalance, m_account_managers[best_ask_exchange].get());
    m_balances.insert_or_assign(best_bid_exchange, std::move(acc_f1.get()));
    m_balances.insert_or_assign(best_ask_exchange, std::move(acc_f2.get()));
  }
private:
  std::mutex m_mutex;
  ArbitrageStrategyOptions m_opts;
  ArbitrageStrategyMatcher m_matcher;
  std::map<std::string, Ticker> m_tickers;
  std::unordered_map<std::string, AccountBalance> m_balances;
  int64_t m_last_trade_us;
};