#pragma once
#include "model/options.h"
#include "model/symbol.h"
#include "model/ticker.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

using namespace std::chrono;

struct ArbitrageStrategyOptions {
  std::unordered_map<std::string, ExchangeParams> exchange_params;
  // TODO: remove
  std::unordered_map<SymbolId, double> default_amount;
  // TODO: get from exchange info endpoint
  std::unordered_map<SymbolId, double> min_amount;
  uint64_t min_trade_interval_us;
  double arbitrage_match_profit_margin;
  std::function<uint64_t(const Ticker&)> time_provider_fcn = [](const Ticker&) { return duration_cast<microseconds>(system_clock::now().time_since_epoch()).count(); };
};
