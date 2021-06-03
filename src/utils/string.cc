#include <iomanip>
#include <string>

namespace cryptobot {

std::string to_string(double num, int precision) {
  std::ostringstream oss;
  oss << std::fixed;
  oss << std::setprecision(precision);
  oss << num;
  return oss.str();
}

bool is_digit(char c) {
  return c >= '0' && c <= '9';
}

size_t precision_from_string(const std::string& str) {
  const size_t sz = str.size();
  size_t i = 0;
  for (; i < sz; ++i) {
    if (str[i] == '.') {
      break;
    }
    if (!is_digit(str[i])) {
      throw std::invalid_argument("precision_from_string: not a number");
    }
  }
  if (i >= sz - 1) {
    return 0;
  }
  size_t j = i + 1;
  size_t last_non_zero_idx = i;
  for (; j < sz; ++j) {
    if (str[j] > '0') {
      last_non_zero_idx = j;
    }
    if (!is_digit(str[j])) {
      throw std::invalid_argument("precision_from_string: not a number");
    }
  }
  return last_non_zero_idx - i;
}

} // namespace cryptobot