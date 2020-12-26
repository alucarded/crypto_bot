#include "trading_strategy.h"

#include <string>

struct BasicStrategyOptions : StrategyOptions {

};

class BasicStrategy : public TradingStrategy {
public:
  BasicStrategy(const BasicStrategyOptions& opts) : m_opts(opts) {

  }

  virtual void execute(const std::map<std::string, Ticker> tickers) override {
    if (tickers.size() < m_opts.m_required_exchanges) {
      std::cout << "Not enough exchanges" << std::endl;
      return;
    }
    std::cout << "Executing strategy" << std::endl;
  }

private:
  BasicStrategyOptions m_opts;
};