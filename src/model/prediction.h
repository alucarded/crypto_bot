#pragma once

#include <cstdint>

enum PriceOutlook : int {
  BEARISH = -1,
  NEUTRAL = 0,
  BULLISH = 1
};

struct Prediction {
  PriceOutlook price_outlook;
  double target_price;
};

struct RangePrediction {
  // By how much will the reference price drop within timeframe_ms time frame
  double min_change;
  // By how much will the reference price rise within timeframe_ms time frame
  double max_change;
  uint64_t timeframe_ms;
};