
#pragma once

#include <iomanip>
#include <string>

namespace cryptobot {

std::string to_string(double num, int precision);
bool is_digit(char c);
size_t precision_from_string(const std::string& str);

}