#include "exchange/exchange_listener.h"
#include "model/prediction.h"
#include "strategy/indicator/relative_strength_index.h"
#include "strategy/trading_signal.hpp"

#include <vector>

struct MarketMakingPredictionData {
  uint64_t timestamp_us;
  // uint64_t interval_ms;
  // std::vector<double> book_imbalances;
  // std::vector<double> trade_volumes;
  // std::vector<double> trade_prices;
};

class MarketMakingSignal : public TradingSignal<MarketMakingPredictionData, MarketMakingPrediction>, public ExchangeListener {
public:
  MarketMakingSignal();
  virtual ~MarketMakingSignal();
  virtual MarketMakingPrediction Predict(const MarketMakingPredictionData& data) const override;

  // ExchangeListener
  virtual void OnConnectionOpen(const std::string& name) override;
  virtual void OnConnectionClose(const std::string& name) override;
  virtual void OnBookTicker(const Ticker& ticker) override;

private:
  RelativeStrengthIndex m_rsi;
  int32_t m_minute;
  Ticker m_book_ticker;
  std::vector<double> m_mid_closes;
};