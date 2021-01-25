#pragma once
#include <string>

enum Side : int {
  ASK = -1,
  BID = 1
};

class ExchangeClient {
public:
  virtual void MarketOrder(const std::string& symbol, Side side, double qty) = 0;
  virtual void LimitOrder(const std::string& symbol, Side side, double qty, double price) = 0;
  virtual void CancelAllOrders() = 0;
};