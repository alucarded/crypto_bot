#include "kraken_client.hpp"

#include <iostream>

int main() {
  KrakenClient kraken_client;
  //kraken_client.MarketOrder("xbtusdt", Side::BID, 0.001);
  auto balance = kraken_client.GetAccountBalance();
  BOOST_LOG_TRIVIAL(info) << balance.GetRawResponse();
  BOOST_LOG_TRIVIAL(info) << "AccountBalance:" << balance.Get();
  if (!balance) {
    BOOST_LOG_TRIVIAL(error) << balance.GetErrorMsg();
  }
}