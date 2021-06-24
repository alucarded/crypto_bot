#pragma once

#include "model/symbol.h"

#include <string>

struct TradeTicker {
  uint64_t event_time;
  uint64_t trade_time;
  SymbolPair symbol;
  std::string exchange;
  std::string trade_id;
  double price;
  double qty;
  // Is the buyer the market maker?
  bool is_market_maker;
  // TODO: always set
  uint64_t arrived_ts;

  friend std::ostream &operator<<(std::ostream &os, const TradeTicker &res);
};