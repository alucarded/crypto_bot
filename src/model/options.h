#pragma once

#include <string>

struct ExchangeParams {
  ExchangeParams(const std::string& exchange_name_, double slippage_, double fee_)
    : exchange_name(exchange_name_), slippage(slippage_), fee(fee_) {

  }

  std::string exchange_name;
  double slippage;
  double fee;
};