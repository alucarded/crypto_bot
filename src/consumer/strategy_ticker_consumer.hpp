#include "strategy/trading_strategy.h"
#include "ticker.h"
#include "ticker_consumer.h"

#include <cassert>
#include <iostream>
#include <string>
#include <map>

class StrategyTickerConsumer : public TickerConsumer {
public:
  StrategyTickerConsumer(TradingStrategy* strategy) : m_strategy(strategy) { }

  virtual void Consume(const RawTicker& raw_ticker) override {
    // std::cout << "Consumed:" << std::endl;
    // std::cout << raw_ticker << std::endl;
    if (raw_ticker.m_bid.empty() || raw_ticker.m_ask.empty()) {
      m_tickers.erase(raw_ticker.m_exchange);
      std::cout << "Empty ticker: " << raw_ticker << std::endl;
    } else {
      Ticker ticker;
      ticker.m_bid = std::stod(raw_ticker.m_bid);
      ticker.m_bid_vol = raw_ticker.m_bid_vol.empty() ? std::nullopt : std::optional<double>(std::stod(raw_ticker.m_bid_vol));
      ticker.m_ask = std::stod(raw_ticker.m_ask);
      ticker.m_ask_vol = raw_ticker.m_ask_vol.empty() ? std::nullopt : std::optional<double>(std::stod(raw_ticker.m_ask_vol));
      ticker.m_source_ts = raw_ticker.m_source_ts ? std::optional<int64_t>(raw_ticker.m_source_ts) : std::nullopt;
      ticker.m_arrived_ts = raw_ticker.m_arrived_ts;
      if (m_tickers.count(raw_ticker.m_exchange)) {
        assert(ticker.m_arrived_ts > m_tickers[raw_ticker.m_exchange].m_arrived_ts);
      }
      m_tickers[raw_ticker.m_exchange] = ticker;
    }
    m_strategy->execute(m_tickers);
  }
private:
  std::map<std::string, Ticker> m_tickers;
  TradingStrategy* m_strategy;
};