#pragma once
#include "ticker.h"
#include "order_book.h"

#include <boost/log/trivial.hpp>

class ExchangeListener {
public:
  virtual void OnConnectionOpen() {
    BOOST_LOG_TRIVIAL(info) << "ExchangeListener::OnConnectionOpen";
  }

  virtual void OnConnectionClose() {
    BOOST_LOG_TRIVIAL(info) << "ExchangeListener::OnConnectionClose";
  }

  virtual void OnTicker(const Ticker& ticker) {
    BOOST_LOG_TRIVIAL(info) << "ExchangeListener::OnTicker";
  }

  virtual void OnOrderBook(const OrderBook& order_book) {
    BOOST_LOG_TRIVIAL(info) << "ExchangeListener::OnOrderBook";
  }

  virtual void OnOrderBookUpdate(const OrderBookUpdate& order_book) {
    BOOST_LOG_TRIVIAL(info) << "ExchangeListener::OnOrderBookUpdate";
  }
};