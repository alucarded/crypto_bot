#pragma once

#include "symbol.h"

#include <optional>
#include <string>
#include <vector>


struct OrderBookUpdate {
  struct Level {
    std::string price;
    std::string volume;
    std::optional<uint64_t> timestamp;
  };

  uint64_t last_update_id;
  bool is_snapshot;
  std::vector<Level> bids;
  std::vector<Level> asks;
  // TODO: always set
  uint64_t arrived_ts;
  std::string exchange;
  SymbolPairId symbol;
};