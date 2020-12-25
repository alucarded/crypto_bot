#pragma once
#include "ticker.h"

#include <map>

class TradingStrategy {
public:
  virtual void execute(const std::map<std::string, Ticker> tickers) = 0;
};