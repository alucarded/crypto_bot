#include "kraken_client.hpp"

int main() {
  KrakenClient kraken_client;
  kraken_client.MarketOrder("xbtusdt", Side::BID, 0.001);
}