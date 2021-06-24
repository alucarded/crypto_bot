#include "model/order_book.h"

class OrderBookImbalance {
public:
  OrderBookImbalance(size_t limit);

  double Calculate(const OrderBook& ob) const;
private:
  size_t m_limit;
};