#pragma once

#include "symbol.h"

#include <cstdint>

enum PriceOutlook : int {
  BEARISH = -1,
  NEUTRAL = 0,
  BULLISH = 1
};

struct Prediction {
  SymbolPairId symbol;
  PriceOutlook price_outlook;
  // Mid price prediction
  double target_price;
  // Base price
  double base_price;
  double confidence;
  uint64_t timeframe_ms;
};

struct MarketMakingPrediction {
  SymbolPairId symbol;
  double base_price;
  // From -1.0 to 1.0; -1.0 means extremely bearish, 1.0 means extremely bullish
  double signal;
  uint64_t timeframe_ms;
};