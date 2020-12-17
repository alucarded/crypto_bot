#include <atomic>

// TODO: FIXME: actually we need atomic struct
struct AtomicTicker {
  std::atomic<double> m_ask;
  std::atomic<double> m_ask_vol;
  std::atomic<double> m_bid;
  std::atomic<double> m_bid_vol;
  std::atomic<int64_t> m_source_ts;
  std::atomic<int64_t> m_arrived_ts;
};