#include "account_manager.hpp"
#include "account_refresher.h"

void AccountRefresher::WatchImpl() {
  m_account_manager->RefreshAccountBalance();
}