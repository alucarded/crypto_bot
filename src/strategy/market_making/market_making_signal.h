#include "model/prediction.h"
#include "strategy/trading_signal.hpp"

#include <vector>

struct MarketMakingPredictionData {
  uint64_t interval_ms;
  std::vector<double> book_imbalances;
  std::vector<double> trade_volumes;
  std::vector<double> trade_prices;
};

class MarketMakingSignal : public TradingSignal<MarketMakingPredictionData, MarketMakingPrediction> {
public:
  virtual ~MarketMakingSignal();
  virtual MarketMakingPrediction Predict(const MarketMakingPredictionData& data) const override;
};