#pragma once

#include "model/account_balance.h"
#include "model/order.h"

#include <boost/log/trivial.hpp>

#include <string>

class UserDataListener {
public:
  virtual void OnConnectionOpen(const std::string& name) {
    BOOST_LOG_TRIVIAL(info) << "UserDataListener::OnConnectionOpen";
  }

  virtual void OnConnectionClose(const std::string& name) {
    BOOST_LOG_TRIVIAL(info) << "UserDataListener::OnConnectionClose";
  }

  /**
   * @param account_balance contains new balances for assets (only assets for which balance changed)
   * 
   */
  virtual void OnAccountBalanceUpdate(const AccountBalance& account_balance) {
    BOOST_LOG_TRIVIAL(info) << "UserDataListener::OnAccountBalanceUpdate";
  }

  virtual void OnOrderUpdate(const Order& order) {
    BOOST_LOG_TRIVIAL(info) << "UserDataListener::OnOrderUpdate";
  }
};