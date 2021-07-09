#include "market_making_signal.h"

#include <chrono>

using namespace std::chrono;

namespace {

inline int32_t GetMinute(uint64_t arrived_ts_us) {
  microseconds us = microseconds(arrived_ts_us);
  minutes mins = duration_cast<minutes>(us);
  return mins.count();
}

}

MarketMakingSignal::MarketMakingSignal() : m_rsi(14), m_minute(0) {

}

MarketMakingSignal::~MarketMakingSignal() {

}

MarketMakingPrediction MarketMakingSignal::Predict(const MarketMakingPredictionData& data) const {
  std::optional<double> rsi_opt = m_rsi.Calculate(m_mid_closes);
  MarketMakingPrediction res;
  res.symbol = m_book_ticker.symbol;
  res.base_price = (m_book_ticker.ask + m_book_ticker.bid)/2.0d;
  res.signal = 0;
  res.timeframe_ms = 1000000;
  res.timestamp_us = data.timestamp_us;
  if (rsi_opt.has_value()) {
    double val = rsi_opt.value();
    if (val > 70.0d || val < 30.0d) {
      res.signal = (val - 50.0d)*2.0d/100.0d;
    }
  }
  return res;
}

void MarketMakingSignal::OnConnectionOpen(const std::string& name) {

}

void MarketMakingSignal::OnConnectionClose(const std::string& name) {

}

void MarketMakingSignal::OnBookTicker(const Ticker& ticker) {
  auto mins_count = GetMinute(ticker.arrived_ts);
  if (m_minute < mins_count) {
    m_mid_closes.push_back((m_book_ticker.bid + m_book_ticker.ask)/2.0d);
    m_minute = mins_count;
  }
  m_book_ticker = ticker;
}