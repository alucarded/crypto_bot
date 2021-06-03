#pragma once

#include "account_refresher.h"
#include "exchange_client.h"
#include "user_data_listener.hpp"
#include "utils/spinlock.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_set>

class AccountManager : public ExchangeClient {
public:
  virtual bool HasOpenOrders() = 0;

  virtual bool HasOpenOrders(SymbolPairId pair) = 0;

  virtual double GetFreeBalance(SymbolId symbol_id) = 0;

  virtual double GetTotalBalance(SymbolId symbol_id) = 0;

  virtual bool IsAccountSynced() const = 0;
};