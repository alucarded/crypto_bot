#include <string>

enum Side : int {
  ASK = -1,
  BID = 1
};

class ExchangeAccount {
public:
  virtual void MarketOrder(const std::string& symbol, Side side, double qty) = 0;
  virtual void LimitOrder(const std::string& symbol, Side side, double qty, double price) = 0;
  virtual void CancelAllOrders() = 0;

  virtual void OnTicker(const Ticker& ticker) = 0;
  virtual void OnDisconnected() = 0;
};