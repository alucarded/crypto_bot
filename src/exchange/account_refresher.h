#pragma once

#include "utils/watcher.h"

using namespace std::chrono;

class AccountManagerImpl;

class AccountRefresher : public Watcher {
public:
  AccountRefresher(AccountManagerImpl* account_manager)
      : Watcher(300000), m_account_manager(account_manager) {

  }

protected:
  virtual void WatchImpl() override;

protected:
  AccountManagerImpl* m_account_manager;
};