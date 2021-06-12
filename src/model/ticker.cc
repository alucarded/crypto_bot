#include "ticker.h"

std::ostream &operator<<(std::ostream &os, const Ticker &res) {
  int precision = std::numeric_limits<double>::max_digits10;
  os << "ask=" << std::setprecision(precision) << res.ask << ", ask_vol="
      << (res.ask_vol ? std::to_string(res.ask_vol.value()) : "null")
      << ", bid=" << std::setprecision(precision) << res.bid
      << ", bid_vol=" << (res.bid_vol ? std::to_string(res.bid_vol.value()) : "null")
      << ", source_ts=" << (res.source_ts ? std::to_string(res.source_ts.value()) : "null")
      << ", arrived_ts=" << std::to_string(res.arrived_ts)
      << ", id=" << std::to_string(res.id)
      << ", exchange=" << res.exchange
      << ", symbol=" << res.symbol;
  return os;
}

uint64_t RawTicker::last_ticker_id = 0;

std::ostream& operator<<(std::ostream& os, const RawTicker& rt) {
  return os << "ask=" << rt.ask
      << ", ask_volume=" << rt.ask_vol
      << ", bid=" << rt.bid
      << ", bid_volume=" << rt.bid_vol
      << ", source_ts=" << std::to_string(rt.source_ts)
      << ", arrived_ts=" << std::to_string(rt.arrived_ts)
      << ", exchange=" << rt.exchange
      << ", symbol=" << rt.symbol;
}
