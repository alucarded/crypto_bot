#pragma once

#include "model/account_balance.h"

class AccountBalanceListener {
public:
  virtual ~AccountBalanceListener() {}

  virtual void OnAccountBalanceUpdate(const AccountBalance& balance) = 0;
};