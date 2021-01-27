#include "client/exchange_client.h"

[[deprecated("These methods are part of TradingStrategy")]]
class ExchangeAccount : public ExchangeClient {
public:
  virtual void OnTicker(const Ticker& ticker) = 0;
  virtual void OnDisconnected() = 0;
};