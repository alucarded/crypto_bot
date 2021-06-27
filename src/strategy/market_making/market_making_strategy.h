#include "exchange/exchange_listener.h"
#include "model/trade_ticker.h"
#include "strategy/indicator/order_book_imbalance.h"

class MarketMakingStrategy : public ExchangeListener {
public:

  MarketMakingStrategy() : m_order_book_imbalance() {

  }

  virtual ~MarketMakingStrategy() {

  }

  virtual void OnConnectionOpen(const std::string& name) override {

  }

  virtual void OnConnectionClose(const std::string& name) override {
    
  }

  virtual void OnBookTicker(const Ticker& ticker) override {
    BOOST_LOG_TRIVIAL(debug) << "MarketMakingStrategy::OnBookTicker, ticker=" << ticker;
  }

  virtual void OnTradeTicker(const TradeTicker& ticker) override {
    BOOST_LOG_TRIVIAL(info) << "MarketMakingStrategy::OnTradeTicker, ticker=" << ticker;
  }

  virtual void OnOrderBookUpdate(const OrderBook& order_book) override {
    BOOST_LOG_TRIVIAL(debug) << "MarketMakingStrategy::OnOrderBookUpdate, order_book=" << order_book;
    m_ob_imbalance.Calculate(order_book);
  }

private:
  OrderBookImbalance m_ob_imbalance;
};