#include "market_making_signal.h"

#include <boost/log/trivial.hpp>

#include <chrono>

using namespace std::chrono;

namespace {

inline int32_t GetMinute(uint64_t arrived_ts_us) {
  microseconds us = microseconds(arrived_ts_us);
  minutes mins = duration_cast<minutes>(us);
  return mins.count();
}

}

MarketMakingSignal::MarketMakingSignal() : m_rsi(14), m_sma_short(8), m_sma_mid(21), m_sma_long(55), m_minute(0) {

}

MarketMakingSignal::~MarketMakingSignal() {

}

MarketMakingPrediction MarketMakingSignal::Predict(const MarketMakingPredictionData& data) const {
  //double imba = m_ob_imbalance.Calculate(data.ob, 0.01);

  // Default response initialization
  MarketMakingPrediction res;
  res.symbol = m_book_ticker.symbol;
  res.base_price = (m_book_ticker.ask + m_book_ticker.bid)/2.0d;
  res.signal = 0;
  res.confidence = 0;
  res.timeframe_ms = 1000000;
  res.timestamp_us = data.timestamp_us;

  // 3 SMAs
  auto short_sma_opt = m_sma_short.Get();
  auto mid_sma_opt = m_sma_mid.Get();
  auto long_sma_opt = m_sma_long.Get();
  auto short_sma_slope_opt = m_sma_short.GetSlope();
  auto mid_sma_slope_opt = m_sma_mid.GetSlope();
  auto long_sma_slope_opt = m_sma_long.GetSlope();
  if (!long_sma_slope_opt.has_value()) {
    BOOST_LOG_TRIVIAL(info) << "Predict: not enough data";
    return res;
  }

  auto short_sma = short_sma_opt.value();
  auto mid_sma = mid_sma_opt.value();
  auto long_sma = long_sma_opt.value();
  auto short_sma_slope = short_sma_slope_opt.value();
  auto mid_sma_slope = mid_sma_slope_opt.value();
  auto long_sma_slope = long_sma_slope_opt.value();

  if (short_sma_slope > 0 && mid_sma_slope > 0 && long_sma_slope > 0
      && short_sma > mid_sma && mid_sma > long_sma) {
    res.signal = 1.0;
    res.confidence = 0.1;
  }

  if (short_sma_slope < 0 && mid_sma_slope < 0 && long_sma_slope < 0
      && short_sma < mid_sma && mid_sma < long_sma) {
    res.signal = -1.0;
    res.confidence = 0.1;
  }

  std::optional<double> rsi_opt = m_rsi.Calculate(m_mid_closes);
  if (rsi_opt.has_value()) {
    double val = rsi_opt.value();
    BOOST_LOG_TRIVIAL(debug) << "RSI: " << val;
    if (val > 70.0d || val < 30.0d) {
      //res.signal = (50.0d - val)*2.0d/100.0d;
      // Block sending orders
      res.confidence = 0;
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
    m_sma_short.Update(m_mid_closes);
    m_sma_mid.Update(m_mid_closes);
    m_sma_long.Update(m_mid_closes);
    m_minute = mins_count;
  }
  m_book_ticker = ticker;
}