
#pragma once

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
}