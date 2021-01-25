#include "client/exchange_client.h"
#include "consumer/consumer.h"
#include "trading_strategy.h"

#include <string>
#include <map>
#include <memory>

struct ArbitrageStrategyOptions {

};

class ArbitrageStrategy : public TradingStrategy, public Consumer<RawTicker> {
public:
    ArbitrageStrategy(const ArbitrageStrategyOptions& opts, ExchangeAccount* exchange_account)
      : m_opts(opts), m_exchange_account(exchange_account) {

  }

  virtual void execute(const std::map<std::string, Ticker>& tickers) override {
  }

  virtual void Consume(const RawTicker& raw_ticker) override {
  }
private:
  ArbitrageStrategyOptions m_opts;
  ExchangeAccount* m_exchange_account;
  std::map<std::string, ExchangeClient*> m_exchange_clients;
};