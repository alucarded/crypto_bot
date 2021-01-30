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
    ArbitrageStrategy(const ArbitrageStrategyOptions& opts, const std::map<std::string, ExchangeClient*>& exchange_clients)
      : m_opts(opts), m_matcher(), m_exchange_clients(exchange_clients) {

  }

  virtual void execute(const std::string& updated_ticker, const std::map<std::string, Ticker>& tickers) override {
    // unused
  }

  virtual void OnTicker(const Ticker& ticker) override {
    std::cout << "Ticker." << std::endl << "Bid: " << std::to_string(ticker.m_bid) << std::endl << "Ask: " << std::to_string(ticker.m_ask) << std::endl;
    m_tickers[ticker.m_exchange] = ticker;
    auto match = m_matcher.FindMatch(m_tickers);
    if (match.has_value()) {
      // TODO:
      std::cout << "Got match" << std::endl;
    }
  }

  virtual void OnDisconnected(const std::string& exchange_name) override {
    m_tickers.erase(exchange_name);
  }

private:
  ArbitrageStrategyOptions m_opts;
  ArbitrageStrategyMatcher m_matcher;
  // TODO: perhaps add to TradingStrategy - make it a base abstract class
  std::map<std::string, ExchangeClient*> m_exchange_clients;
  std::map<std::string, Ticker> m_tickers;
};