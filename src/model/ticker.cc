#include "ticker.h"

std::ostream &operator<<(std::ostream &os, const Ticker &res) {
  int precision = std::numeric_limits<double>::max_digits10;
  os << "m_ask=" << std::setprecision(precision) << res.m_ask << ", m_ask_vol="
      << (res.m_ask_vol ? std::to_string(res.m_ask_vol.value()) : "null")
      << ", m_bid=" << std::setprecision(precision) << res.m_bid
      << ", m_bid_vol=" << (res.m_bid_vol ? std::to_string(res.m_bid_vol.value()) : "null")
      << ", m_source_ts=" << (res.m_source_ts ? std::to_string(res.m_source_ts.value()) : "null")
      << ", m_arrived_ts=" << std::to_string(res.m_arrived_ts)
      << ", m_id=" << std::to_string(res.m_id)
      << ", m_exchange=" << res.m_exchange
      << ", m_symbol=" << res.m_symbol;
  return os;
}

uint64_t RawTicker::m_last_ticker_id = 0;

std::ostream& operator<<(std::ostream& os, const RawTicker& rt) {
  return os << "m_ask=" << rt.m_ask
      << ", m_ask_volume=" << rt.m_ask_vol
      << ", m_bid=" << rt.m_bid
      << ", m_bid_volume=" << rt.m_bid_vol
      << ", m_source_ts=" << std::to_string(rt.m_source_ts)
      << ", m_arrived_ts=" << std::to_string(rt.m_arrived_ts)
      << ", m_exchange=" << rt.m_exchange
      << ", m_symbol=" << rt.m_symbol;
}
