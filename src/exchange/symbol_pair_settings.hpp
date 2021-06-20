#pragma once

#include "model/symbol.h"

#include <algorithm>
#include <string>

struct SymbolPairSettings {
  std::string symbol;
  std::string base_asset;
  std::string quote_asset;
  size_t quote_precision;
  size_t base_asset_precision;
  size_t quote_asset_precision;
  double min_price;
  double max_price;
  double tick_size;
  double min_qty;
  double max_qty;
  double step_size;
  // Precision derived from tick_size
  size_t price_precision;
  // Precision derived from step size
  size_t order_precision;

  std::string GetLowerCaseSymbol() const {
    std::string ret(symbol);
    std::for_each(ret.begin(), ret.end(), [](char & c) {
      c = ::tolower(c);
    });
    return ret;
  }
};