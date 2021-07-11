#include "relative_strength_index.h"

#include <boost/log/trivial.hpp>

#include <cassert>

RelativeStrengthIndex::RelativeStrengthIndex(size_t period) : m_period(period) {
  assert(period > 0);
}

namespace {

inline void CalcuateChanges(double prev, double curr, double& u, double& d) {
  if (curr > prev) {
    u = curr - prev;
    d = 0;
  } else {
    u = 0;
    d = prev - curr;
  }
}

};

std::optional<double> RelativeStrengthIndex::Calculate(const std::vector<double>& close_values) const {
  size_t sz = close_values.size();
  if (sz <= m_period) {
    BOOST_LOG_TRIVIAL(debug) << "[RelativeStrengthIndex::Calculate] Insufficient data: has " << sz << " periods, requires " << m_period + 1;
    return std::nullopt;
  }
  size_t first_idx = sz - m_period - 1;
  double u, d, smma_u, smma_d;
  CalcuateChanges(close_values[first_idx], close_values[first_idx+1], u, d);
  smma_u = u;
  smma_d = d;
  for (size_t i = first_idx + 2; i < sz; ++i) {
    CalcuateChanges(close_values[i-1], close_values[i], u, d);
    smma_u = (smma_u*(m_period - 1) + u)/m_period;
    smma_d = (smma_d*(m_period - 1) + d)/m_period;
  }
  return smma_d == 0 ? 100.0d : (100.0d - 100.0d/(1.0d + smma_u/smma_d));
}