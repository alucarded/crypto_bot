#include "account_manager.h"
#include "exchange_client.h"

class Exchange : public AccountManager, public ExchangeClient {
};