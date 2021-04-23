#include <cassert>
#include <optional>
#include <vector>

class SimpleMovingAverage {
public:
  SimpleMovingAverage(size_t period) : m_period(period), m_count(0) {
  }

  void Update(const std::vector<double>& series) {
    size_t sz = series.size();
    assert(sz >= m_count);
    ++m_count;
    m_sum += series[sz - 1];
    if (m_count > m_period) {
      m_sum -= series[sz - 1 - m_period];
    }
    if (m_count >= m_period) {
      m_smas.push_back(m_sum / double(m_period));
    }
  }

  void Reset() {
    m_smas.clear();
    m_sum = 0;
    m_count = 0;
  }

  void Calculate(const std::vector<double>& series) {
    // TODO:
  }

  std::optional<double> Get() const {
    if (m_smas.empty()) {
      // TODO: use optional or
      return std::nullopt;
    }
    return std::optional<double>(m_smas[m_smas.size() - 1]);
  }

  inline double GetUnsafe() const {
    return m_smas[m_smas.size() - 1];
  }

  std::optional<double> GetSlope() const {
    size_t sz = m_smas.size();
    if (sz < 2) {
      return std::nullopt;
    }
    return std::optional<double>(m_smas[sz - 1] - m_smas[sz - 2]);
  }

  inline double GetSlopeUnsafe() const {
    const size_t sz = m_smas.size();
    return m_smas[sz - 1] - m_smas[sz - 2];
  }
private:
  const size_t m_period;
  std::vector<double> m_smas;
  double m_sum;
  size_t m_count;
};