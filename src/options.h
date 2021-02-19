#pragma once

#include <string>

struct ExchangeParams {
  ExchangeParams(const std::string& exchange_name, double slippage, double fee)
    : m_exchange_name(exchange_name), m_slippage(slippage), m_fee(fee) {

  }

  std::string m_exchange_name;
  double m_slippage;
  double m_fee;
};