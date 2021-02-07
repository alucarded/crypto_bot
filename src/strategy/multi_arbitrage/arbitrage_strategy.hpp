#include "arbitrage_strategy_matcher.hpp"

#include "client/exchange_client.h"
#include "consumer/consumer.h"
#include "strategy/trading_strategy.h"
#include "ticker_subscriber.h"

#include <string>
#include <map>
#include <memory>
#include <unordered_map>
#include <utility>

struct ArbitrageStrategyOptions {

};

class ArbitrageStrategy : public TradingStrategy, public TickerSubscriber {
public:
    ArbitrageStrategy(const ArbitrageStrategyOptions& opts)
      : m_opts(opts), m_matcher() {

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
      const auto& best_bid_exchange = match.m_best_bid.m_exchange;
      const auto& best_ask_exchange = match.m_best_ask.m_exchange;
      std::cout << "Got match between " << best_bid_exchange << " and " << best_ask_exchange
        << " with profit " << std::to_string(match.m_profit) << std::endl;
      if (m_exchange_clients.count(best_bid_exchange) > 0
        && m_exchange_clients.count(best_ask_exchange) > 0
        // Make sure we have new tickers after sending last order
        && m_is_fresh[best_bid_exchange]
        && m_is_fresh[best_ask_exchange]) {
        // TODO: 1) Write sent orders, so that they can be later compared with executed trades
        // TODO: 2) After sending an order to an exchange block until we got new ticker AND the previous order is executed
        // TODO: 3) For safety, add adjustable timer to wait until another order is sent ?
        m_is_fresh[best_bid_exchange] = false;
        m_is_fresh[best_ask_exchange] = false;
        m_exchange_clients[best_bid_exchange]->MarketOrder("BTCUSDT", Side::ASK, 0.001);
        m_exchange_clients[best_ask_exchange]->MarketOrder("BTCUSDT", Side::BID, 0.001);
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