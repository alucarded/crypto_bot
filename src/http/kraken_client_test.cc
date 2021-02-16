#include "kraken_client.hpp"

#include <iostream>

int main() {
  KrakenClient kraken_client;
  //kraken_client.MarketOrder("xbtusdt", Side::BID, 0.001);
  auto balance = kraken_client.GetAccountBalance();
  std::cout << balance;
}