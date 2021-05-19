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
  std::unordered_map<std::string, ExchangeParams> m_exchange_params;
  std::unordered_map<SymbolId, double> m_default_amount;
  std::unordered_map<SymbolId, double> m_min_amount;
  int64_t m_max_ticker_age_us;
  int64_t m_max_ticker_delay_us;
  int64_t m_min_trade_interval_us;
  std::function<uint64_t(const Ticker&)> time_provider_fcn = [](const Ticker& ticker) { return duration_cast<microseconds>(system_clock::now().time_since_epoch()).count(); };
};
