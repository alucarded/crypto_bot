#include "binance_client.hpp"

#include <iostream>

int main() {
  BinanceClient binance_client;
  //kraken_client.MarketOrder("xbtusdt", Side::BID, 0.001);
  auto balance = binance_client.GetAccountBalance();
  BOOST_LOG_TRIVIAL(info) << balance.GetRawResponse();
  BOOST_LOG_TRIVIAL(info) << "AccountBalance:" << balance.Get();
  if (!balance) {
    BOOST_LOG_TRIVIAL(error) << balance.GetErrorMsg();
  }
}