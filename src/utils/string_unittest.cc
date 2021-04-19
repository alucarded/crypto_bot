#include "string.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace testing;
using namespace cryptobot;

TEST(CryptobotUtilsTest, PrecisionFromStringTest) {
  EXPECT_EQ(0, precision_from_string("1.0000000"));
  EXPECT_EQ(5, precision_from_string("0.00001"));
  EXPECT_EQ(3, precision_from_string("0.00100000"));
  EXPECT_EQ(0, precision_from_string("0"));
  EXPECT_EQ(5, precision_from_string("0.1234500"));
  EXPECT_EQ(1, precision_from_string("0.1"));
  EXPECT_EQ(0, precision_from_string("128"));
  try {
    precision_from_string("asdf");
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {

  } catch(...) {
    FAIL() << "Expected std::invalid_argument";
  }
}