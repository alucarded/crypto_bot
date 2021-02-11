#pragma once
#include <string>

enum Side : int {
  ASK = -1,
  BID = 1
};

struct NewOrderResult {
  NewOrderResult(const std::string& response) : m_full_response(response) {}
  std::string m_full_response;
  // TODO:

  friend std::ostream &operator<<(std::ostream &os, const NewOrderResult &res);
};

std::ostream &operator<<(std::ostream &os, const NewOrderResult &res) {
  os << res.m_full_response;
  return os;
}

class ExchangeClient {
public:
  virtual NewOrderResult MarketOrder(const std::string& symbol, Side side, double qty) = 0;
  virtual NewOrderResult LimitOrder(const std::string& symbol, Side side, double qty, double price) = 0;
  virtual void CancelAllOrders() = 0;
};