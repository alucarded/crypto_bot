#pragma once
#include "ticker.h"

#include <map>

struct StrategyOptions {
  std::string m_trading_exchange = "binance";
  size_t m_required_exchanges = 8;
};

class TradingStrategy {
public:
  [[deprecated]]
  virtual void execute(const std::string& updated_ticker, const std::map<std::string, Ticker>& tickers) = 0;

  virtual void OnTicker(const Ticker& ticker) = 0;
  virtual void OnDisconnected(const std::string& exchange_name) = 0;
};