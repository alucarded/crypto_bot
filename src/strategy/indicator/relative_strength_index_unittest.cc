#include "relative_strength_index.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace testing;

TEST(RelativeStrengthIndexTest, BasicTest) {
  RelativeStrengthIndex rsi(3);
  std::vector<double> closes{3, 4, 2, 5};
  std::optional<double> rsi_val = rsi.Calculate(closes);
  EXPECT_TRUE(rsi_val.has_value());
  EXPECT_NEAR(76.47059, rsi_val.value(), 0.00001);
}