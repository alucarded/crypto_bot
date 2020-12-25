#include "strategy/trading_strategy.h"
#include "ticker.h"
#include "ticker_consumer.h"

#include <iostream>
#include <unordered_map>

class StrategyTickerConsumer : public TickerConsumer {
public:
  StrategyTickerConsumer(TradingStrategy* strategy) : m_strategy(strategy) { }

  virtual void Consume(const RawTicker& raw_ticker) override {
    // std::cout << "Consumed:" << std::endl;
    // std::cout << raw_ticker << std::endl;
    if (m_tickers.count(raw_ticker.m_exchange)) {

    }
    // TODO
    m_strategy->execute(m_tickers);
  }
private:
  std::unordered_map<std::string, Ticker> m_tickers;
  TradingStrategy* m_strategy;
};