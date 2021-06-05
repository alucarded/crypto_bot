#pragma once
#include "model/ticker.h"
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

  virtual void OnTradeTicker(const Ticker& ticker) {
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
    ticker.m_ask = double(best_ask.GetPrice())/cryptobot::quick_pow10(precision_settings.m_price_precision);
    ticker.m_ask_vol = std::optional<double>(best_ask.GetVolume());
    ticker.m_bid = double(best_bid.GetPrice())/cryptobot::quick_pow10(precision_settings.m_price_precision);
    ticker.m_bid_vol = std::optional<double>(best_bid.GetVolume());
    ticker.m_source_ts = std::optional<int64_t>(std::min(best_ask.GetTimestamp(), best_bid.GetTimestamp()));
    using namespace std::chrono;
    auto now = system_clock::now();
    system_clock::duration tp = now.time_since_epoch();
    microseconds us = duration_cast<microseconds>(tp);
    ticker.m_arrived_ts = us.count();
    ticker.m_exchange = order_book.GetExchangeName();
    ticker.m_symbol = order_book.GetSymbolPairId();
    //BOOST_LOG_TRIVIAL(info) << "Arrived: " << ticker.m_arrived_ts << ", best ask: " << best_ask.GetTimestamp() << ", best bid: " << best_bid.GetTimestamp() << std::endl;
    //BOOST_LOG_TRIVIAL(info) << ticker << std::endl;
    return ticker;
  }
};