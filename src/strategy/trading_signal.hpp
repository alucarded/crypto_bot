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
  virtual Prediction Predict() const = 0;
};