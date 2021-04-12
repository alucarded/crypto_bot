enum PriceOutlook : int {
  BEARISH = -1,
  NEUTRAL = 0,
  BULLISH = 1
};

class TradingSignal {
public:
  virtual PriceOutlook GetPRiceOutlook() const = 0;
};