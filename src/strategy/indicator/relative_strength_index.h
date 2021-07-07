#include <optional>
#include <vector>

class RelativeStrengthIndex {
public:
  RelativeStrengthIndex(size_t period);

  std::optional<double> Calculate(const std::vector<double>& close_values) const;
private:
  size_t m_period;
};