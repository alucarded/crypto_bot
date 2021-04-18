#include "simple_moving_average.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <optional>
#include <vector>

using namespace testing;

TEST(SimpleMovingAverageTest, UpdateTest) {
  std::vector<double> series;
  SimpleMovingAverage sma{3};

  series.push_back(63575.0);
  sma.Update(series);
  EXPECT_EQ(false, sma.Get().has_value());
  EXPECT_EQ(false, sma.GetSlope().has_value());

  series.push_back(62959.53);
  sma.Update(series);
  EXPECT_EQ(false, sma.Get().has_value());
  EXPECT_EQ(false, sma.GetSlope().has_value());

  series.push_back(63159.98);
  sma.Update(series);
  EXPECT_EQ(true, sma.Get().has_value());
  EXPECT_NEAR(63231.50, sma.Get().value(), 0.005);
  EXPECT_EQ(false, sma.GetSlope().has_value());

  series.push_back(61334.80);
  sma.Update(series);
  EXPECT_EQ(true, sma.Get().has_value());
  EXPECT_NEAR(62484.77, sma.Get().value(), 0.005);
  EXPECT_EQ(true, sma.GetSlope().has_value());
  EXPECT_NEAR(-746.73, sma.GetSlope().value(), 0.005);
}