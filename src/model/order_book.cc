#include "order_book.h"

std::ostream& operator<<(std::ostream& os, const PriceLevel& pl) {
  os << "{ \"price\": " << std::to_string(uint64_t(pl.m_price)) << ", \"volume\": " << std::fixed << std::setprecision(8) << pl.m_volume << ", \"timestamp\": " << std::to_string(pl.m_timestamp) << " }";
  return os;
}

std::ostream& operator<<(std::ostream& os, const OrderBook& ob) {
  os << "ASKS:" << std::endl;
  for (const auto& lvl : ob.m_asks) {
    os << lvl << std::endl;
  }
  os << "BIDS:" << std::endl;
  for (const auto& lvl : ob.m_bids) {
    os << lvl << std::endl;
  }
  return os;
}