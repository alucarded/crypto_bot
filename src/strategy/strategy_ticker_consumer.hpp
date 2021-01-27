#include "client/exchange_client.h"
#include "consumer/consumer.h"
#include "ticker.h"
#include "trading_strategy.h"

#include <string>
#include <map>
#include <memory>

class StrategyTickerConsumer : public Consumer<RawTicker> {
public:
    StrategyTickerConsumer(TradingStrategy* strategy)
      : m_strategy(strategy) {

  }

  virtual void Consume(const RawTicker& raw_ticker) override {
    // std::cout << "Consumed:" << std::endl;
    // std::cout << raw_ticker << std::endl;
    // Any empty ticker from an exchange discards validity of its data
    if (raw_ticker.m_bid.empty() || raw_ticker.m_ask.empty()) {
      std::cout << "Empty ticker: " << raw_ticker << std::endl;
      m_strategy->OnDisconnected(raw_ticker.m_exchange);
    } else {
      Ticker ticker;
      ticker.m_bid = std::stod(raw_ticker.m_bid);
      ticker.m_bid_vol = raw_ticker.m_bid_vol.empty() ? std::nullopt : std::optional<double>(std::stod(raw_ticker.m_bid_vol));
      ticker.m_ask = std::stod(raw_ticker.m_ask);
      ticker.m_ask_vol = raw_ticker.m_ask_vol.empty() ? std::nullopt : std::optional<double>(std::stod(raw_ticker.m_ask_vol));
      ticker.m_source_ts = raw_ticker.m_source_ts ? std::optional<int64_t>(raw_ticker.m_source_ts) : std::nullopt;
      ticker.m_arrived_ts = raw_ticker.m_arrived_ts;
      ticker.m_symbol = raw_ticker.m_symbol;
      // if (m_tickers.count(raw_ticker.m_exchange)) {
      //   assert(ticker.m_arrived_ts > m_tickers[raw_ticker.m_exchange].m_arrived_ts);
      // }
      // if (!ProcessTicker(raw_ticker.m_exchange, ticker)) {
      //   return;
      // }
      m_strategy->OnTicker(ticker);
    }
  }
private:
  TradingStrategy* m_strategy;
};