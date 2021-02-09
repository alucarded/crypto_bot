#pragma once
#include "client/exchange_client.h"
#include "ticker.h"

#include <map>
#include <memory>

[[deprecated]]
struct StrategyOptions {
  std::string m_trading_exchange = "binance";
  size_t m_required_exchanges = 8;
};

class TradingStrategy {
public:
  [[deprecated]]
  virtual void execute(const std::string& updated_ticker, const std::map<std::string, Ticker>& tickers) = 0;

  void RegisterExchangeClient(const std::string& exchange_name, ExchangeClient* exchange_client) {
    m_exchange_clients.insert(std::make_pair(exchange_name, std::unique_ptr<ExchangeClient>(exchange_client)));
  }
protected:
  // TODO: use enum instead of string for key
  std::map<std::string, std::unique_ptr<ExchangeClient>> m_exchange_clients;
};