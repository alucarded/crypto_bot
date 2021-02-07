#pragma once
#include <string>

enum Side : int {
  ASK = -1,
  BID = 1
};

struct MarketOrderResult {
  MarketOrderResult(const std::string& response) : m_full_response(response) {}
  std::string m_full_response;
  // TODO:

  friend std::ostream &operator<<(std::ostream &os, const MarketOrderResult &res);
};

std::ostream &operator<<(std::ostream &os, const MarketOrderResult &res) {
  os << res.m_full_response;
  return os;
}

class ExchangeClient {
public:
  virtual MarketOrderResult MarketOrder(const std::string& symbol, Side side, double qty) = 0;
  virtual void LimitOrder(const std::string& symbol, Side side, double qty, double price) = 0;
  virtual void CancelAllOrders() = 0;
};