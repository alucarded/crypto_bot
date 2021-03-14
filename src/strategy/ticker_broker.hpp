#include "consumer/consumer.h"
#include "model/ticker.h"
#include "ticker_subscriber.h"
#include "trading_strategy.h"

#include <initializer_list>
#include <map>
#include <memory>
#include <string>
#include <vector>

// TODO: Remove
class TickerBroker : public Consumer<RawTicker> {
public:
  TickerBroker(std::initializer_list<TickerSubscriber*> subscribers) : m_subscribers(subscribers), m_last_ticker_id(0) {

  }

  virtual void Consume(const RawTicker& raw_ticker) override {
    // std::cout << "Consumed:" << std::endl;
    // std::cout << raw_ticker << std::endl;
    // Any empty ticker from an exchange discards validity of its data
    if (raw_ticker.m_bid.empty() || raw_ticker.m_ask.empty()) {
      //std::cout << "Empty ticker: " << raw_ticker << std::endl;
      for (auto subscriber : m_subscribers) {
        subscriber->OnDisconnected(raw_ticker.m_exchange);
      }
    } else {
      Ticker ticker;
      ticker.m_bid = std::stod(raw_ticker.m_bid);
      ticker.m_bid_vol = raw_ticker.m_bid_vol.empty() ? std::nullopt : std::optional<double>(std::stod(raw_ticker.m_bid_vol));
      ticker.m_ask = std::stod(raw_ticker.m_ask);
      ticker.m_ask_vol = raw_ticker.m_ask_vol.empty() ? std::nullopt : std::optional<double>(std::stod(raw_ticker.m_ask_vol));
      ticker.m_source_ts = raw_ticker.m_source_ts ? std::optional<int64_t>(raw_ticker.m_source_ts) : std::nullopt;
      ticker.m_arrived_ts = raw_ticker.m_arrived_ts;
      ticker.m_symbol = raw_ticker.m_symbol;
      ticker.m_exchange = raw_ticker.m_exchange;
      ticker.m_id = m_last_ticker_id++;
      // if (m_tickers.count(raw_ticker.m_exchange)) {
      //   assert(ticker.m_arrived_ts > m_tickers[raw_ticker.m_exchange].m_arrived_ts);
      // }
      // if (!ProcessTicker(raw_ticker.m_exchange, ticker)) {
      //   return;
      // }
      for (auto subscriber : m_subscribers) {
        subscriber->OnTicker(ticker);
      }
    }
  }
private:
  std::vector<TickerSubscriber*> m_subscribers;
  uint64_t m_last_ticker_id;
};