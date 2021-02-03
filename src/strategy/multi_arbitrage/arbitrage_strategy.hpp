#include "arbitrage_strategy_matcher.hpp"

#include "client/exchange_client.h"
#include "consumer/consumer.h"
#include "strategy/trading_strategy.h"

#include <string>
#include <map>
#include <memory>
#include <utility>

struct ArbitrageStrategyOptions {

};

class ArbitrageStrategy : public TradingStrategy {
public:
    ArbitrageStrategy(const ArbitrageStrategyOptions& opts)
      : m_opts(opts), m_matcher() {

  }

  virtual void execute(const std::string& updated_ticker, const std::map<std::string, Ticker>& tickers) override {
    // unused
  }

  virtual void OnTicker(const Ticker& ticker) override {
    //std::cout << "Ticker." << std::endl << "Bid: " << std::to_string(ticker.m_bid) << std::endl << "Ask: " << std::to_string(ticker.m_ask) << std::endl;
    m_tickers[ticker.m_exchange] = ticker;
    auto match_opt = m_matcher.FindMatch(m_tickers);
    if (match_opt.has_value()) {
      auto match = match_opt.value();
      // TODO: add match writer - write somewhere for analysis
      std::cout << "Got match between " << match.m_best_bid.m_exchange << " and " << match.m_best_ask.m_exchange
        << " with profit " << std::to_string(match.m_profit) << std::endl;
      if (m_exchange_clients.count(match.m_best_bid.m_exchange) > 0
        && m_exchange_clients.count(match.m_best_ask.m_exchange) > 0) {
        
      }
    
    }
  }

  virtual void OnDisconnected(const std::string& exchange_name) override {
    m_tickers.erase(exchange_name);
  }

private:
  ArbitrageStrategyOptions m_opts;
  ArbitrageStrategyMatcher m_matcher;
  std::map<std::string, Ticker> m_tickers;
};