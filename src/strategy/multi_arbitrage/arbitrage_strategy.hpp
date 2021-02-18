#include "arbitrage_strategy_matcher.hpp"

#include "http/exchange_client.h"
#include "consumer/consumer.h"
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
      if (m_exchange_clients.count(best_bid_exchange) > 0
        && m_exchange_clients.count(best_ask_exchange) > 0) {
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
        auto f1 = std::async(std::launch::async, &ExchangeClient::LimitOrder, m_exchange_clients[best_bid_exchange].get(),
            "BTCUSDT", Side::ASK, vol, best_bid_ticker.m_bid);
        auto f2 = std::async(std::launch::async, &ExchangeClient::LimitOrder, m_exchange_clients[best_ask_exchange].get(),
            "BTCUSDT", Side::BID, vol, best_ask_ticker.m_ask);
        m_last_trade_us = now_us;
        BOOST_LOG_TRIVIAL(info) << match << std::endl << "Response from " << best_bid_exchange << ": " << std::endl << f1.get() << std::endl
            << "Response from " << best_ask_exchange << ": " << std::endl << f2.get() << std::endl;
      }
    }
  }

  virtual void OnDisconnected(const std::string& exchange_name) override {
    std::unique_lock<std::mutex> scoped_lock(m_mutex);
    m_tickers.erase(exchange_name);
  }

private:
  std::mutex m_mutex;
  ArbitrageStrategyOptions m_opts;
  ArbitrageStrategyMatcher m_matcher;
  std::map<std::string, Ticker> m_tickers;
  int64_t m_last_trade_us;
};