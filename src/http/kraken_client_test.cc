#include "kraken_client.hpp"

#include <iostream>

int main() {
  KrakenClient kraken_client;
  BOOST_LOG_TRIVIAL(info) << "GetAccountBalance";
  auto balance = kraken_client.GetAccountBalance();
  BOOST_LOG_TRIVIAL(info) << balance.GetRawResponse();
  BOOST_LOG_TRIVIAL(info) << "AccountBalance:" << balance.Get();
  if (!balance) {
    BOOST_LOG_TRIVIAL(error) << balance.GetErrorMsg();
  }
  BOOST_LOG_TRIVIAL(info) << "GetOpenOrders";
  auto orders = kraken_client.GetOpenOrders("BTCUSDT");
  BOOST_LOG_TRIVIAL(info) << orders.GetRawResponse();
  for (const auto& order : orders.Get()) {
    BOOST_LOG_TRIVIAL(info) << "Order: " << order.m_id;
  }
  if (!orders) {
    BOOST_LOG_TRIVIAL(error) << orders.GetErrorMsg();
  }
}