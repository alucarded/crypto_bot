#pragma once

#include "model/account_balance.h"

class AccountBalanceListener {
public:
  virtual void OnAccountBalanceUpdate(const AccountBalance& balance) = 0;
};