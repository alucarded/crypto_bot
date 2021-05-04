#include "model/candle.h"

#include <algorithm>
#include <vector>

class AverageTrueRange {
public:
  AverageTrueRange(size_t period) : m_period(period) {

  }

  void Update(const Candle& c) {
    const size_t sz = m_true_range.size();
    int oldest_idx = static_cast<int>(sz) - static_cast<int>(m_period);
    if (oldest_idx >= 0) {
      m_tr_sum -= m_true_range[oldest_idx];
    }
    // TODO: using current open instead of previous close only because we assume that previous close is current open
    m_true_range.push_back(std::max({c.GetHigh() - c.GetLow(), std::abs(c.GetHigh() - c.GetOpen()), std::abs(c.GetLow() - c.GetOpen())}));
    m_tr_sum += m_true_range[sz];
    m_atr.push_back(m_tr_sum/double(m_period));
  }

  double Get() const {
    return m_atr.at(m_atr.size() - 1);
  }
private:
  const size_t m_period;
  std::vector<double> m_true_range;
  std::vector<double> m_atr;
  double m_tr_sum;
};