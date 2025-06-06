#pragma once

#include <string>

struct ExchangeParams {
  ExchangeParams() { }
  ExchangeParams(const std::string& exchange_name_, double slippage_, double fee_, double daily_volume_)
    : exchange_name(exchange_name_), slippage(slippage_), fee(fee_), daily_volume(daily_volume_) {

  }

  std::string exchange_name;
  double slippage;
  double fee;
  // Daily exchange traded volume in billion USD
  double daily_volume;
  uint64_t max_ticker_age_us;
  uint64_t max_ticker_delay_us;
};