#include "arbitrage_strategy_matcher.hpp"

#include "client/exchange_client.h"
#include "consumer/consumer.h"
#include "strategy/trading_strategy.h"
#include "ticker_subscriber.h"

#include <boost/log/trivial.hpp>

#include <future>
#include <string>
#include <map>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <utility>


struct ArbitrageStrategyOptions {
  std::unordered_map<std::string, ExchangeParams> m_exchange_params;
  double m_default_amount;
};

class ArbitrageStrategy : public TradingStrategy, public TickerSubscriber {
public:
    ArbitrageStrategy(const ArbitrageStrategyOptions& opts)
      : m_opts(opts), m_matcher(opts.m_exchange_params) {
        // Validations
        if (m_opts.m_exchange_params.size() < 2) {
          throw std::invalid_argument("Parameters for at least 2 exchanges required");
        }
        if (m_opts.m_default_amount < 0.0 || m_opts.m_default_amount > 0.1) {
          throw std::invalid_argument("Invalid order amount, please check configuration or change hardcoded limits");
        }
  }

  virtual void execute(const std::string& updated_ticker, const std::map<std::string, Ticker>& tickers) override {
    // unused
  }

  virtual void OnTicker(const Ticker& ticker) override {
    //std::cout << "Ticker." << std::endl << "Bid: " << std::to_string(ticker.m_bid) << std::endl << "Ask: " << std::to_string(ticker.m_ask) << std::endl;
    m_is_fresh[ticker.m_exchange] = true;
    m_tickers[ticker.m_exchange] = ticker;
    auto match_opt = m_matcher.FindMatch(m_tickers);
    if (match_opt.has_value()) {
      auto match = match_opt.value();
      // TODO: add match writer - write somewhere for analysis
      const auto& best_bid_ticker = match.m_best_bid;
      const auto& best_ask_ticker = match.m_best_ask;
      const auto& best_bid_exchange = best_bid_ticker.m_exchange;
      const auto& best_ask_exchange = best_ask_ticker.m_exchange;
      std::cout << "Got match between " << best_bid_exchange << " and " << best_ask_exchange
        << " with profit " << std::to_string(match.m_profit) << std::endl;
      if (m_exchange_clients.count(best_bid_exchange) > 0
        && m_exchange_clients.count(best_ask_exchange) > 0
        // Make sure we have new tickers after sending last order
        && m_is_fresh[best_bid_exchange]
        && m_is_fresh[best_ask_exchange]) {
        // TODO: After sending an order to an exchange block until we got new ticker AND the previous order is executed
        // TODO: For safety, add adjustable timer to wait until another order is sent ?
        m_is_fresh[best_bid_exchange] = false;
        m_is_fresh[best_ask_exchange] = false;
        double vol = m_opts.m_default_amount;
        if (best_bid_ticker.m_bid_vol.has_value()) {
          vol = std::min(best_bid_ticker.m_bid_vol.value(), vol);
        }
        if (best_ask_ticker.m_ask_vol.has_value()) {
          vol = std::min(best_ask_ticker.m_ask_vol.value(), vol);
        }
        auto f1 = std::async(std::launch::async, &ExchangeClient::MarketOrder, m_exchange_clients[best_bid_exchange].get(),
            "BTCUSDT", Side::ASK, vol);
        auto f2 = std::async(std::launch::async, &ExchangeClient::MarketOrder, m_exchange_clients[best_ask_exchange].get(),
            "BTCUSDT", Side::BID, vol);
        BOOST_LOG_TRIVIAL(info) << match << std::endl << "Response from " << best_bid_exchange << ": " << std::endl << f1.get() << std::endl
            << "Response from " << best_ask_exchange << ": " << std::endl << f2.get() << std::endl;
      }
    
    }
  }

  virtual void OnDisconnected(const std::string& exchange_name) override {
    m_tickers.erase(exchange_name);
  }

private:
  ArbitrageStrategyOptions m_opts;
  ArbitrageStrategyMatcher m_matcher;
  std::unordered_map<std::string, bool> m_is_fresh;
  std::map<std::string, Ticker> m_tickers;
};