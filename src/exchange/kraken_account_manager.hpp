#include "account_manager.hpp"
#include "http/kraken_client.hpp"

class KrakenAccountManager : public AccountManager {
public:
  KrakenAccountManager(KrakenClient* kraken_client) : AccountManager(kraken_client) {
  }

  virtual ~KrakenAccountManager() {
  }

};