#include "exchange_client.h"
#include "binapi/api.hpp"

#include <curl/curl.h>

namespace {

// HMAC_SHA256, Good_guy_12
// Do not worry, these are testnet keys
const std::string g_api_key = "44pNgqvCmQcPAEjNY4I96TflqFRf4oQaQUbmkbxISdKGGmJojJ1kH8LkOYy6JIUD";
const std::string g_secret = "giavUBTzRVa91h58H8GhQ0UWqfGFW7Mu93PF5oXPG6AMKWIlbpbpQ6E9YTDycOAJ";

}

class BinanceClient : public ExchangeClient {
public:
  BinanceClient(const std::string& base_url) : m_base_url(base_url){

  }

  virtual ~BinanceClient() {

  }

  virtual void MarketOrder(const std::string& symbol, Side side, double qty) override {

  }

  virtual void LimitOrder(const std::string& symbol, Side side, double qty, double price) override {

  }

  virtual void CancelAllOrders() override {

  }

private:
  std::string m_base_url;
};