#include <optional>

class SimpleMovingAverage {
public:
  SimpleMovingAverage(size_t period) : m_period(period) {
  }

  void Add(double val) {
    const size_t curr_size = m_values.size();

    // TODO: right now it will grow indefinietly, add memory cleanup logic
    m_values.push_back(val);
    m_sum += val;
    if (curr_size >= m_period) {
      if (curr_size > m_period) {
        m_sum -= m_values[curr_size - 1 - m_period];
      }
      m_smas.push_back(m_sum / double(m_period));
    }
  }

  std::optional<double> Get() const {
    if (m_smas.empty()) {
      // TODO: use optional or
      return std::nullopt;
    }
    return std::optional<double>(m_smas[m_smas.size() - 1]);
  }

  std::optional<double> GetSlope() const {
    size_t sz = m_smas.size();
    if (sz < 2) {
      return std::nullopt;
    }
    return std::optional<double>(m_smas[sz - 1] - m_smas[sz - 2]);
  }
private:
  const size_t m_period;
  std::vector<double> m_values;
  std::vector<double> m_smas;
  double m_sum;
};