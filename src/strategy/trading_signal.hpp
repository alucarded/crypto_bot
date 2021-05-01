enum PriceOutlook : int {
  BEARISH = -1,
  NEUTRAL = 0,
  BULLISH = 1
};

struct Prediction {
  PriceOutlook price_outlook;
  double target_price;
};

class TradingSignal {
public:
// TODO: it is arbitrage-specific now..
  virtual Prediction Predict(double best_bid, double best_ask) const = 0;
};