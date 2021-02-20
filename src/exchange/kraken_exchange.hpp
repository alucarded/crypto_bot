#include "exchange.h"

#include "http/kraken_client.hpp"

/**
 * This proxies KrakenClient.
 */
class KrakenExchange : public Exchange {
public:
  virtual NewOrderResult MarketOrder(const std::string& symbol, Side side, double qty) override { }
  virtual NewOrderResult LimitOrder(const std::string& symbol, Side side, double qty, double price) override { }
  virtual AccountBalance GetAccountBalance() override { }
  virtual void CancelAllOrders() override { }
};