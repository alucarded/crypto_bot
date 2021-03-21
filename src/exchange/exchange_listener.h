#pragma once
#include "model/ticker.h"
#include "model/order_book.h"

#include <boost/log/trivial.hpp>

class ExchangeListener {
public:
  virtual void OnConnectionOpen(const std::string& name) {
    BOOST_LOG_TRIVIAL(info) << "ExchangeListener::OnConnectionOpen, name=" + name;
  }

  virtual void OnConnectionClose(const std::string& name) {
    BOOST_LOG_TRIVIAL(info) << "ExchangeListener::OnConnectionClose, name=" + name;
  }

  virtual void OnTicker(const Ticker& ticker) {
    BOOST_LOG_TRIVIAL(info) << "ExchangeListener::OnTicker, ticker=" << ticker;
  }

  virtual void OnOrderBookUpdate(const OrderBook& order_book) {
    BOOST_LOG_TRIVIAL(info) << "ExchangeListener::OnOrderBookUpdate, order_book=" << order_book;
  }
};