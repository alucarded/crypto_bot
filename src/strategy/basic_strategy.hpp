#include "trading_strategy.h"

#include <string>

class BasicStrategy : public TradingStrategy {
public:
  BasicStrategy(std::string trading_exchange) : m_trading_exchange(trading_exchange) {

  }

  virtual void execute(const std::unordered_map<std::string, Ticker> tickers) override {

  }

private:
  const std::string m_trading_exchange;
};