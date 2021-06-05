#pragma once

#include "connection_listener.h"

#include "model/account_balance.h"
#include "model/order.h"

#include <boost/log/trivial.hpp>

#include <string>

class UserDataListener : public ConnectionListener {
public:
  /**
   * @param account_balance contains new balances for assets (only assets for which balance changed)
   * 
   */
  virtual void OnAccountBalanceUpdate(const AccountBalance& account_balance) = 0;

  virtual void OnOrderUpdate(const Order& order) = 0;
};