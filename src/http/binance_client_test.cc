#include "binance_client.hpp"

#include <iostream>

int main() {
  BinanceClient binance_client;
  //kraken_client.MarketOrder("xbtusdt", Side::BID, 0.001);
  auto balance = binance_client.GetAccountBalance();
  std::cout << balance;
}