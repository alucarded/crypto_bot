#include "client/exchange_client.h"

class ExchangeAccount : public ExchangeClient {
public:
  virtual void OnTicker(const Ticker& ticker) = 0;
  virtual void OnDisconnected() = 0;
};