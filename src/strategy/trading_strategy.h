#pragma once
#include "exchange/account_manager.hpp"
#include "ticker.h"

#include <map>
#include <memory>

class TradingStrategy {
public:
  void RegisterExchangeClient(const std::string& exchange_name, ExchangeClient* exchange) {
    m_account_managers.insert(std::make_pair(exchange_name, std::unique_ptr<AccountManager>(new AccountManager(exchange))));
  }
protected:
  // TODO: use enum instead of string for key
  std::map<std::string, std::unique_ptr<AccountManager>> m_account_managers;
};