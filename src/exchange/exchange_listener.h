#pragma once
#include "connection_listener.h"

#include "model/ticker.h"
#include "model/trade_ticker.h"
#include "model/order_book.h"
#include "utils/math.h"

#include <boost/log/trivial.hpp>

// TODO: split into separate interfaces (ExchangeConnectionListener, BookTickerListener, TradeTickerListener, OrderBookListener) ??
// ^ Interface Segregation Principle
class ExchangeListener : public ConnectionListener {
public:
  virtual void OnBookTicker(const Ticker& ticker) {
    BOOST_LOG_TRIVIAL(info) << "ExchangeListener::OnBookTicker, ticker=" << ticker;
  }

  virtual void OnTradeTicker(const TradeTicker& ticker) {
    BOOST_LOG_TRIVIAL(info) << "ExchangeListener::OnTradeTicker, ticker=" << ticker;
  }

  virtual void OnOrderBookUpdate(const OrderBook& order_book) {
    BOOST_LOG_TRIVIAL(info) << "ExchangeListener::OnOrderBookUpdate, order_book=" << order_book;
  }

  static Ticker TickerFromOrderBook(const OrderBook& order_book) {
    const auto& best_ask = order_book.GetBestAsk();
    const auto& best_bid = order_book.GetBestBid();
    const auto& precision_settings = order_book.GetPrecisionSettings();
    Ticker ticker;
    ticker.ask = double(best_ask.GetPrice())/cryptobot::quick_pow10(precision_settings.m_price_precision);
    ticker.ask_vol = std::optional<double>(best_ask.GetVolume());
    ticker.bid = double(best_bid.GetPrice())/cryptobot::quick_pow10(precision_settings.m_price_precision);
    ticker.bid_vol = std::optional<double>(best_bid.GetVolume());
    ticker.source_ts = std::optional<int64_t>(std::min(best_ask.GetTimestamp(), best_bid.GetTimestamp()));
    using namespace std::chrono;
    auto now = system_clock::now();
    system_clock::duration tp = now.time_since_epoch();
    microseconds us = duration_cast<microseconds>(tp);
    ticker.arrived_ts = us.count();
    ticker.exchange = order_book.GetExchangeName();
    ticker.symbol = order_book.GetSymbolPairId();
    //BOOST_LOG_TRIVIAL(info) << "Arrived: " << ticker.arrived_ts << ", best ask: " << best_ask.GetTimestamp() << ", best bid: " << best_bid.GetTimestamp() << std::endl;
    //BOOST_LOG_TRIVIAL(info) << ticker << std::endl;
    return ticker;
  }
};