#include "consumer/consumer.h"
#include "exchange_account.hpp"


class BacktestExchangeAccount : public ExchangeAccount {
  virtual void MarketOrder(const std::string& symbol, Side side) override {

  }

  virtual void LimitOrder(const std::string& symbol, Side side, double price) override {

  }

  virtual void CancelAllOrders() override {

  }

  virtual void OnTicker(const Ticker& ticker) override {

  }

  virtual void OnDisconnected() override {

  }
};