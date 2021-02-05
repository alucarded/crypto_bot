#pragma once
#include "exchange_account.hpp"
#include "ticker.h"

#include <map>

[[deprecated]]
struct StrategyOptions {
  std::string m_trading_exchange = "binance";
  size_t m_required_exchanges = 8;
};

class TradingStrategy {
public:
  [[deprecated]]
  virtual void execute(const std::string& updated_ticker, const std::map<std::string, Ticker>& tickers) = 0;

  virtual void OnTicker(const Ticker& ticker) = 0;
  virtual void OnDisconnected(const std::string& exchange_name) = 0;

  void RegisterExchangeClient(const std::string& exchange_name, ExchangeAccount* exchange_account) {
    m_exchange_clients.insert(std::make_pair(exchange_name, std::unique_ptr<ExchangeAccount>(exchange_account)));
  }
protected:
  // TODO: use enum instead of string for key
  std::map<std::string, std::unique_ptr<ExchangeAccount>> m_exchange_clients;
};