#pragma once

#include "utils/watcher.h"

using namespace std::chrono;

class AccountManager;

class AccountRefresher : public Watcher {
public:
  AccountRefresher(AccountManager* account_manager)
      : Watcher(300000), m_account_manager(account_manager) {

  }

protected:
  virtual void WatchImpl() override;

protected:
  AccountManager* m_account_manager;
};