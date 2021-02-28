#pragma once
#include "ticker.h"
#include "order_book.h"

#include <boost/log/trivial.hpp>

class ExchangeListener {
public:
  virtual void OnConnectionOpen(const std::string& name) {
    BOOST_LOG_TRIVIAL(info) << "ExchangeListener::OnConnectionOpen";
  }

  virtual void OnConnectionClose(const std::string& name) {
    BOOST_LOG_TRIVIAL(info) << "ExchangeListener::OnConnectionClose";
  }

  virtual void OnTicker(const Ticker& ticker) {
    BOOST_LOG_TRIVIAL(info) << "ExchangeListener::OnTicker";
  }

  virtual void OnOrderBookUpdate(const OrderBook& order_book) {
    BOOST_LOG_TRIVIAL(info) << "ExchangeListener::OnOrderBookUpdate";
  }
};