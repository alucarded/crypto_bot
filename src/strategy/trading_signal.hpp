#pragma once

#include "model/prediction.h"

template <typename Input>
class TradingSignal {
public:
  virtual RangePrediction Predict(const Input& input) const = 0;
};