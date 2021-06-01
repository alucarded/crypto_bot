#pragma once

#include "model/account_balance.h"

class AccountBalanceListener {
public:
  virtual void OnBalanceUpdate(const AccountBalance& balance) = 0;
};