#pragma once

#include "model/prediction.h"

template <typename Input, typename Output>
class TradingSignal {
public:
  virtual Output Predict(const Input& input) const = 0;
};