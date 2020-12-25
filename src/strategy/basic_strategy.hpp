#include "trading_strategy.h"

#include <string>

struct StrategyOptions {
  std::string m_trading_exchange = "binance";
  size_t m_required_exchanges = 8;
};

class BasicStrategy : public TradingStrategy {
public:
  BasicStrategy(const StrategyOptions& opts) : m_opts(opts) {

  }

  virtual void execute(const std::map<std::string, Ticker> tickers) override {
    if (tickers.size() < m_opts.m_required_exchanges) {
      std::cout << "Not enough exchanges" << std::endl;
      return;
    }
    std::cout << "Executing strategy" << std::endl;
  }

private:
  StrategyOptions m_opts;
};