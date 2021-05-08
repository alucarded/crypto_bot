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
  double m_ask;
  std::optional<double> m_ask_vol;
  double m_bid;
  std::optional<double> m_bid_vol;
  std::optional<int64_t> m_source_ts;
  int64_t m_arrived_ts;
  uint64_t m_id;
  std::string m_exchange;
  SymbolPair m_symbol;

  friend std::ostream &operator<<(std::ostream &os, const Ticker &res);
};

std::ostream &operator<<(std::ostream &os, const Ticker &res) {
  int precision = std::numeric_limits<double>::max_digits10;
  os << "m_ask=" << std::setprecision(precision) << res.m_ask << ", m_ask_vol="
      << (res.m_ask_vol ? std::to_string(res.m_ask_vol.value()) : "null")
      << ", m_bid=" << std::setprecision(precision) << res.m_bid
      << ", m_bid_vol=" << (res.m_bid_vol ? std::to_string(res.m_bid_vol.value()) : "null")
      << ", m_source_ts=" << (res.m_source_ts ? std::to_string(res.m_source_ts.value()) : "null")
      << ", m_arrived_ts=" << std::to_string(res.m_arrived_ts)
      << ", m_id=" << std::to_string(res.m_id)
      << ", m_exchange=" << res.m_exchange
      << ", m_symbol=" << res.m_symbol;
  return os;
}

struct RawTicker {
  std::string m_ask;
  std::string m_ask_vol;
  std::string m_bid;
  std::string m_bid_vol;
  int64_t m_source_ts;
  int64_t m_arrived_ts;
  std::string m_exchange;
  std::string m_symbol;
  static uint64_t m_last_ticker_id;

  static RawTicker Empty(const std::string& exchange) {
    RawTicker empty_ticker;
    empty_ticker.m_exchange = exchange;
    using namespace std::chrono;
    auto now = system_clock::now();
    system_clock::duration tp = now.time_since_epoch();
    microseconds us = duration_cast<microseconds>(tp);
    empty_ticker.m_source_ts = us.count();
    empty_ticker.m_ask = "";
    empty_ticker.m_ask_vol = "";
    empty_ticker.m_bid = "";
    empty_ticker.m_bid_vol = "";
    return empty_ticker;
  }

  friend std::ostream& operator<<(std::ostream& os, const RawTicker& rt);
};

uint64_t RawTicker::m_last_ticker_id = 0;

std::ostream& operator<<(std::ostream& os, const RawTicker& rt) {
  return os << "m_ask=" << rt.m_ask
      << ", m_ask_volume=" << rt.m_ask_vol
      << ", m_bid=" << rt.m_bid
      << ", m_bid_volume=" << rt.m_bid_vol
      << ", m_source_ts=" << std::to_string(rt.m_source_ts)
      << ", m_arrived_ts=" << std::to_string(rt.m_arrived_ts)
      << ", m_exchange=" << rt.m_exchange
      << ", m_symbol=" << rt.m_symbol;
}

struct TradeTicker {
  // TODO
};