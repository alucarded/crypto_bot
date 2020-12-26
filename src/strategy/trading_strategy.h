#pragma once
#include "ticker.h"

#include <map>

struct StrategyOptions {
  std::string m_trading_exchange = "binance";
  size_t m_required_exchanges = 8;
};

class TradingStrategy {
public:
  virtual void execute(const std::map<std::string, Ticker>& tickers) = 0;
};