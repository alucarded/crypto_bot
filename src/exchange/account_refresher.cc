#include "account_manager_impl.h"
#include "account_refresher.h"

void AccountRefresher::WatchImpl() {
  m_account_manager->RefreshAccountBalance();
}