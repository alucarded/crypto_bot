#include "model/candle.h"
#include "trading_signal.hpp"

#include <cassert>

using namespace std::chrono;

struct MeanReversionSignalSettings {
  size_t moving_average_period_minutes;
};

class MeanReversionSignal : public TradingSignal {
public:

  MeanReversionSignal(const MeanReversionSignalSettings& settings)
      : m_settings(settings), m_now_minute(0) {

  }

  virtual PriceOutlook GetPriceOutlook() const override {
    auto slope_opt = m_sma.GetSlope();
    if (!slope_opt.has_value()) {
      return PriceOutlook::NEUTRAL;
    }
    auto slope_val = slope_opt.value();
    auto sma_opt = m_sma.Get();
    auto sma_val = sma_opt.value();
    auto current_price = m_candle.GetClose();
    // TODO: improve with some statistical model
    if (slope_val > 0) {
      if (current_price > sma_val) {
        // 1. Above SMA and upward trend
        return PriceOutlook::NEUTRAL;
      } else {
        // 2. Below SMA and upward trend
        return PriceOutlook::BULLISH;
      }
    } else {
      if (current_price > sma_val) {
        // 3. Above SMA and downward trend
        return PriceOutlook::BEARISH;
      } else {
        // 2. Below SMA and downward trend
        return PriceOutlook::NEUTRAL;
      }
    }
    return PriceOutlook::NEUTRAL;
  }

  virtual double GetDirectionalBias() const override {
    // TODO: implement
    return double(GetPriceOutlook());
  }

  void Consume(const Ticker& ticker) {
    auto now = system_clock::now();
    system_clock::duration tp = now.time_since_epoch();
    minutes mins = duration_cast<minutes>(tp);
    auto mins_count = mins.count();
    double m_mean_price = (ticker.m_bid + ticker.m_ask) / 2;
    // TODO: use spread and volatility somehow ?
    if (m_minute < mins_count) {
      m_candles.push_back(m_candle);
      m_sma.Add(m_candle.GetClose());
      // TODO: add trade volume
      m_candle.Reset(m_mean_price, 0);
      return;
    }
    assert(m_minute == mins_count);
    m_candle.Add(m_mean_price, 0);
  }

private:
  MeanReversionSignalSettings m_settings;
  uint64_t m_minute;
  // TODO: not sure if we need a candle here
  Candle m_candle;
  double m_sum;
  std::vector<Candle> m_candles;
  SimpleMovingAverage m_sma;
};