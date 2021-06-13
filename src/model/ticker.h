#pragma once

#include "model/symbol.h"

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>

// TODO: use this
enum ExchangeId : int {
  BINANCE,
  COINBASE,
  KRAKEN
};

struct Ticker {
  double ask;
  std::optional<double> ask_vol;
  double bid;
  std::optional<double> bid_vol;
  std::optional<uint64_t> source_ts;
  uint64_t arrived_ts;
  uint64_t id;
  std::string exchange;
  SymbolPair symbol;

  Ticker& operator=(const Ticker& o) {
    ask = o.ask;
    ask_vol = o.ask_vol;
    bid = o.bid;
    bid_vol = o.bid_vol;
    source_ts = o.source_ts;
    arrived_ts = o.arrived_ts;
    id = o.id;
    exchange = o.exchange;
    symbol = o.symbol;
    return *this;
  }

  friend std::ostream &operator<<(std::ostream &os, const Ticker &res);
};

struct RawTicker {
  std::string ask;
  std::string ask_vol;
  std::string bid;
  std::string bid_vol;
  uint64_t source_ts;
  uint64_t arrived_ts;
  std::string exchange;
  std::string symbol;
  static uint64_t last_ticker_id;

  static RawTicker Empty(const std::string& exchange) {
    RawTicker empty_ticker;
    empty_ticker.exchange = exchange;
    using namespace std::chrono;
    auto now = system_clock::now();
    system_clock::duration tp = now.time_since_epoch();
    microseconds us = duration_cast<microseconds>(tp);
    empty_ticker.source_ts = us.count();
    empty_ticker.ask = "";
    empty_ticker.ask_vol = "";
    empty_ticker.bid = "";
    empty_ticker.bid_vol = "";
    return empty_ticker;
  }

  friend std::ostream& operator<<(std::ostream& os, const RawTicker& rt);
};