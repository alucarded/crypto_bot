#pragma once
#include "exchange/account_manager.hpp"
#include "model/ticker.h"

#include <map>
#include <memory>

class TradingStrategy {
public:
  void RegisterExchangeClient(const std::string& exchange_name, AccountManager* account_manager) {
    m_account_managers.insert(std::make_pair(exchange_name, std::unique_ptr<AccountManager>(account_manager)));
  }
protected:
  // TODO: use enum instead of string for key
  std::map<std::string, std::unique_ptr<AccountManager>> m_account_managers;
};