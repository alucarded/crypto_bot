#include "account_manager.hpp"
#include "http/binance_client.hpp"

class BinanaceAccountManager : public AccountManager {
public:
  BinanaceAccountManager(BinanceClient* binance_client) : AccountManager(binance_client) {
  }

  virtual ~BinanaceAccountManager() {
  }

};