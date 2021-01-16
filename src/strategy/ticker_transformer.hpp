#include "ticker.h"

#include <cassert>
#include <optional>

class TickerTransformer {
public:
  virtual bool Transform(Ticker& ticker) = 0;
};

class CoinbaseTickerTransformer : public TickerTransformer {
public:
  virtual bool Transform(Ticker& ticker) override {
    if (ticker.m_symbol == "USDT-USD") {
      m_usdt_usd = ticker;
    } else if (m_usdt_usd && ticker.m_symbol == "BTC-USD") {
      ticker.m_ask = ticker.m_ask / m_usdt_usd->m_ask;
      ticker.m_bid = ticker.m_bid / m_usdt_usd->m_ask;
      return true;
    }
    return false;
  }
private:
  std::optional<Ticker> m_usdt_usd;
};