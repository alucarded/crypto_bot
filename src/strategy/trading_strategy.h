#pragma once
#include "ticker.h"

#include <unordered_map>

class TradingStrategy {
public:
  virtual void execute(const std::unordered_map<std::string, Ticker> tickers) = 0;
};