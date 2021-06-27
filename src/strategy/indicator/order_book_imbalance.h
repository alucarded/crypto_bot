#include "model/order_book.h"

class OrderBookImbalance {
public:
  OrderBookImbalance();

  double Calculate(const OrderBook& ob, size_t limit) const;
  double Calculate(const OrderBook& ob, double depth_coeff) const;
};