#include "kraken_client.hpp"

#include <iostream>

int main() {
  KrakenClient kraken_client;
  // BOOST_LOG_TRIVIAL(info) << "LimitOrder";
  // auto new_order = kraken_client.LimitOrder("BTCUSDT", Side::BUY, 0.002, 40000.0);
  // BOOST_LOG_TRIVIAL(info) << new_order.GetRawResponse();
  // BOOST_LOG_TRIVIAL(info) << "Order: " << new_order.Get();
  // if (!new_order) {
  //   BOOST_LOG_TRIVIAL(error) << new_order.GetErrorMsg();
  // }
  BOOST_LOG_TRIVIAL(info) << "GetAccountBalance";
  auto balance = kraken_client.GetAccountBalance();
  BOOST_LOG_TRIVIAL(info) << balance.GetRawResponse();
  BOOST_LOG_TRIVIAL(info) << "AccountBalance:" << balance.Get();
  if (!balance) {
    BOOST_LOG_TRIVIAL(error) << balance.GetErrorMsg();
  }
  BOOST_LOG_TRIVIAL(info) << "GetOpenOrders";
  auto orders = kraken_client.GetOpenOrders();
  BOOST_LOG_TRIVIAL(info) << orders.GetRawResponse();
  for (const auto& order : orders.Get()) {
    BOOST_LOG_TRIVIAL(info) << "Order: " << order;
  }
  if (!orders) {
    BOOST_LOG_TRIVIAL(error) << orders.GetErrorMsg();
  }
  auto token = kraken_client.GetWebSocketsToken();
  BOOST_LOG_TRIVIAL(info) << "Token: " << token;
}