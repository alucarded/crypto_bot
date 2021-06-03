#pragma once
#include "exchange/account_manager.h"
#include "model/ticker.h"

#include <memory>
#include <unordered_map>

class TradingStrategy {
public:
  void RegisterExchangeClient(const std::string& exchange_name, std::shared_ptr<AccountManager> account_manager) {
    m_account_managers.insert(std::make_pair(exchange_name, std::move(account_manager)));
  }
protected:
  std::unordered_map<std::string, std::shared_ptr<AccountManager>> m_account_managers;
};