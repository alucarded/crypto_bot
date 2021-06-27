#include "order_book_imbalance.h"

#include <boost/log/trivial.hpp>

#include <cmath>

OrderBookImbalance::OrderBookImbalance() {

}

double OrderBookImbalance::Calculate(const OrderBook& ob, size_t limit) const {
  const auto& bids = ob.GetBids();
  const auto& asks = ob.GetAsks();
  double bid_vol_sum = 0;
  size_t depth = 0;
  for (auto it = bids.begin(); it != bids.end() && depth < limit; ++it, ++depth) {
    bid_vol_sum += it->GetVolume();
  }
  double ask_vol_sum = 0;
  for (auto it = asks.begin(); it != asks.end(); ++it) {
    ask_vol_sum += it->GetVolume();
  }
  return bid_vol_sum / (bid_vol_sum + ask_vol_sum);
}

double OrderBookImbalance::Calculate(const OrderBook& ob, double depth_coeff) const {
  const auto& best_bid = ob.GetBestBid();
  const auto& best_ask = ob.GetBestAsk();
  price_t min_bid = price_t(static_cast<uint64_t>((1.0 - depth_coeff) * static_cast<uint64_t>(best_bid.GetPrice())));
  price_t max_ask = price_t(static_cast<uint64_t>((1.0 + depth_coeff) * static_cast<uint64_t>(best_ask.GetPrice())));

  const auto& bids = ob.GetBids();
  const auto& asks = ob.GetAsks();
  BOOST_LOG_TRIVIAL(trace) << "min_bid=" << uint64_t(min_bid) << ", max_ask=" << uint64_t(max_ask) << ", first bid = " << uint64_t(bids.begin()->GetPrice()) << ", last bid = " << uint64_t(bids.rbegin()->GetPrice());
  BOOST_LOG_TRIVIAL(trace) <<"first ask = " << uint64_t(asks.begin()->GetPrice()) << ", last ask = " << uint64_t(asks.rbegin()->GetPrice());
  double bid_vol_sum = 0;
  for (auto it = bids.rbegin(); it != bids.rend() && it->GetPrice() >= min_bid; ++it) {
    bid_vol_sum += it->GetVolume();
  }
  double ask_vol_sum = 0;
  for (auto it = asks.rbegin(); it != asks.rend() && it->GetPrice() <= max_ask; ++it) {
    ask_vol_sum += it->GetVolume();
  }
  return bid_vol_sum / (bid_vol_sum + ask_vol_sum);
}