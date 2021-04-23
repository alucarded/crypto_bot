#include "indicator/simple_moving_average.hpp"
#include "model/candle.h"
#include "trading_signal.hpp"

#include <boost/log/trivial.hpp>

#include <cassert>
#include <optional>

using namespace std::chrono;

struct MeanReversionSignalSettings {
  MeanReversionSignalSettings(size_t sh, size_t mid, size_t lng)
      : short_sma_period(sh), mid_sma_period(mid), long_sma_period(lng) {}
  size_t short_sma_period;
  size_t mid_sma_period;
  size_t long_sma_period;
};

class MeanReversionSignal : public TradingSignal {
public:

  MeanReversionSignal()
      : m_minute(0), m_sma_short(8), m_sma_mid(21), m_sma_long(55) {

  }

  MeanReversionSignal(const MeanReversionSignalSettings& settings)
      : m_minute(0), m_sma_short(settings.short_sma_period), m_sma_mid(settings.mid_sma_period),
        m_sma_long(settings.long_sma_period) {

  }

  MeanReversionSignal(size_t short_sma_period, size_t mid_sma_period, size_t long_sma_period)
      : m_minute(0), m_sma_short(short_sma_period), m_sma_mid(mid_sma_period), m_sma_long(long_sma_period) {

  }

  virtual Prediction Predict() const override {
    double current_price = (m_ticker.m_bid + m_ticker.m_ask) / 2;

    auto short_sma_opt = m_sma_short.Get();
    auto mid_sma_opt = m_sma_mid.Get();
    auto long_sma_opt = m_sma_long.Get();

    auto short_sma_slope_opt = m_sma_short.GetSlope();
    auto mid_sma_slope_opt = m_sma_mid.GetSlope();
    auto long_sma_slope_opt = m_sma_long.GetSlope();

    if (!short_sma_opt.has_value() || !mid_sma_opt.has_value() || !long_sma_opt.has_value()
        || !short_sma_slope_opt.has_value() || !mid_sma_slope_opt.has_value() || !long_sma_slope_opt.has_value()) {
      BOOST_LOG_TRIVIAL(info) << "Predict: not enough data";
      return {PriceOutlook::NEUTRAL, current_price};
    }

    auto short_sma = short_sma_opt.value();
    auto mid_sma = mid_sma_opt.value();
    auto long_sma = long_sma_opt.value();

    auto short_sma_slope = short_sma_slope_opt.value();
    auto mid_sma_slope = mid_sma_slope_opt.value();
    auto long_sma_slope = long_sma_slope_opt.value();
  
    // We try to take advantage of longer term momentum (trend measured as SMA slope) and shorter term mean reversion
    // TODO: better way of finding target price (perhaps use some stochastic method and create couple of orders)
    // TODO: ...and also set threshold instead of current_price (eg. use ATR)
    auto take_profit_price = GetTargetPrice();
    if (short_sma > current_price && mid_sma > current_price && long_sma > current_price
        && short_sma_slope > 0 && mid_sma_slope > 0 && long_sma_slope > 0) {
      BOOST_LOG_TRIVIAL(info) << "Bullish prediction: " << take_profit_price;
      return {PriceOutlook::BULLISH, take_profit_price};
    }
    if (short_sma < current_price && mid_sma < current_price && long_sma < current_price
        && short_sma_slope < 0 && mid_sma_slope < 0 && long_sma_slope < 0) {
      BOOST_LOG_TRIVIAL(info) << "Bearish prediction: " << take_profit_price;
      return {PriceOutlook::BEARISH, take_profit_price};
    }
    BOOST_LOG_TRIVIAL(info) << "Neutral prediction";
    return {PriceOutlook::NEUTRAL, current_price};
  }

  void Consume(const Ticker& ticker) {
    auto now = system_clock::now();
    system_clock::duration tp = now.time_since_epoch();
    minutes mins = duration_cast<minutes>(tp);
    auto mins_count = mins.count();
    if (m_minute < mins_count) {
      double mean_price = (ticker.m_bid + ticker.m_ask) / 2;
      BOOST_LOG_TRIVIAL(info) << "Minute close for " << ticker.m_symbol << ": " << mean_price;
      m_closes.push_back(mean_price);
      m_sma_short.Update(m_closes);
      m_sma_mid.Update(m_closes);
      m_sma_long.Update(m_closes);
      m_minute = mins_count;
    }
    m_ticker = ticker;
    // TODO: use spread and volatility somehow ?
    // if (m_minute < mins_count) {
    //   m_candles.push_back(m_candle);
    //   m_sma.Add(m_candle.GetClose());
    //   // TODO: add trade volume
    //   m_candle.Reset(mean_price, 0);
    //   return;
    // }
    // assert(m_minute == mins_count);
    // m_candle.Add(mean_price, 0);
  }
private:
  
  double GetTargetPrice() const {
    assert(m_sma_mid.Get().has_value());
    assert(m_sma_long.Get().has_value());
    auto mid_sma = m_sma_mid.GetUnsafe();
    auto long_sma = m_sma_long.GetUnsafe();
    // assert(m_sma_mid.GetSlope().has_value());
    // assert(m_sma_long.GetSlope().has_value());
    // auto mid_sma_slope = m_sma_mid.GetSlopeUnsafe();
    // auto long_sma_slope = m_sma_long.GetSlopeUnsafe();  
    return (mid_sma + long_sma) / 2;
  }

private:
  uint64_t m_minute;
  // // TODO: update candles in a separate CandleHistory object
  // Candle m_candle;
  // std::vector<Candle> m_candles;
  std::vector<double> m_closes;
  SimpleMovingAverage m_sma_short;
  SimpleMovingAverage m_sma_mid;
  SimpleMovingAverage m_sma_long;
  Ticker m_ticker;
};