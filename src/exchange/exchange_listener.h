#pragma once
#include "model/ticker.h"
#include "model/order_book.h"
#include "utils/math.hpp"

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

  static Ticker TickerFromOrderBook(const OrderBook& order_book) {
    const auto& best_ask = order_book.GetBestAsk();
    const auto& best_bid = order_book.GetBestBid();
    const auto& precision_settings = order_book.GetPrecisionSettings();
    Ticker ticker;
    ticker.m_ask = double(best_ask.GetPrice())/quick_pow10(precision_settings.m_price_precision);
    ticker.m_ask_vol = std::optional<double>(best_ask.GetVolume());
    ticker.m_bid = double(best_bid.GetPrice())/quick_pow10(precision_settings.m_price_precision);
    ticker.m_bid_vol = std::optional<double>(best_bid.GetVolume());
    ticker.m_source_ts = std::optional<int64_t>(std::min(best_ask.GetTimestamp(), best_bid.GetTimestamp()));
    using namespace std::chrono;
    auto now = system_clock::now();
    system_clock::duration tp = now.time_since_epoch();
    microseconds us = duration_cast<microseconds>(tp);
    ticker.m_arrived_ts = us.count();
    ticker.m_exchange = order_book.GetExchangeName();
    // TODO: hardcoded
    ticker.m_symbol = order_book.GetSymbolPairId();
    //BOOST_LOG_TRIVIAL(info) << "Arrived: " << ticker.m_arrived_ts << ", best ask: " << best_ask.GetTimestamp() << ", best bid: " << best_bid.GetTimestamp() << std::endl;
    //BOOST_LOG_TRIVIAL(info) << ticker << std::endl;
    return ticker;
  }
};