#include "exchange_client.h"

#include <boost/asio/io_context.hpp>
#include <boost/log/trivial.hpp>

class DummyClient : public ExchangeClient {
public:

  DummyClient() {
  }

  virtual ~DummyClient() {

  }

  virtual std::string GetExchange() override {
    return "";
  }

  virtual Result<Order> MarketOrder(SymbolPairId symbol, Side side, double qty) override {
    return Result<Order>("", Order());
  }

  virtual Result<Order> LimitOrder(SymbolPairId symbol, Side side, double qty, double price) override {
    return Result<Order>("", Order());
  }

  virtual void CancelAllOrders() override {
  }

  virtual Result<AccountBalance> GetAccountBalance() override {
    return Result<AccountBalance>("", AccountBalance());
  }

  virtual Result<std::vector<Order>> GetOpenOrders(SymbolPairId symbol) override {
    return Result<std::vector<Order>>("", std::vector<Order>());
  }
};