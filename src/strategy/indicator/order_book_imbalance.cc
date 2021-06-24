#include "order_book_imbalance.h"

#include <boost/log/trivial.hpp>

#include <cmath>

OrderBookImbalance::OrderBookImbalance(size_t limit) : m_limit(limit) {

}

double OrderBookImbalance::Calculate(const OrderBook& ob) const {
  const auto& bids = ob.GetBids();
  const auto& asks = ob.GetAsks();
  double bid_vol_sum = 0;
  size_t depth = 0;
  for (auto it = bids.begin(); it != bids.end() && depth < m_limit; ++it, ++depth) {
    bid_vol_sum += it->GetVolume();
  }
  double ask_vol_sum = 0;
  for (auto it = asks.begin(); it != asks.end(); ++it) {
    ask_vol_sum += it->GetVolume();
  }
  return bid_vol_sum / (bid_vol_sum + ask_vol_sum);
}