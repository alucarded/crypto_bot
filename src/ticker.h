#include <cstdint>

struct Ticker {
  double m_ask;
  double m_ask_vol;
  double m_bid;
  double m_bid_vol;
  int64_t m_source_ts;
  int64_t m_arrived_ts;
  uint64_t m_id;
};