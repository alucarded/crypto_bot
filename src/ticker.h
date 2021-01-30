#pragma once
#include <chrono>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>

// TODO: use this
enum Exchange : int {
  BINANCE,
  COINBASE
};

struct Ticker {
  // Ticker(double ask, double ask_vol, double bid, double bid_vol, int64_t source_ts, int64_t arrived_ts) :
  //     m_ask(ask), m_ask_vol(ask_vol), m_bid(bid), m_bid_vol(bid_vol), m_source_ts(source_ts), m_arrived_ts(arrived_ts) {

  // }

  double m_ask;
  std::optional<double> m_ask_vol;
  double m_bid;
  std::optional<double> m_bid_vol;
  std::optional<int64_t> m_source_ts;
  int64_t m_arrived_ts;
  uint64_t m_id;
  std::string m_exchange;
  std::string m_symbol;
};

struct RawTicker {
  std::string m_ask;
  std::string m_ask_vol;
  std::string m_bid;
  std::string m_bid_vol;
  int64_t m_source_ts;
  int64_t m_arrived_ts;
  std::string m_exchange;
  std::string m_symbol;

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

std::ostream& operator<<(std::ostream& os, const RawTicker& rt) {
    return os << "Ask: " << rt.m_ask << std::endl
        << "Ask Volume: " << rt.m_ask_vol << std::endl
        << "Bid: " << rt.m_bid << std::endl
        << "Bid Volume: " << rt.m_bid_vol << std::endl
        << "Source Timestamp: " << std::to_string(rt.m_source_ts) << std::endl
        << "Arrived Timestamp: " << std::to_string(rt.m_arrived_ts) << std::endl
        << "Exchange: " << rt.m_exchange << std::endl;
}