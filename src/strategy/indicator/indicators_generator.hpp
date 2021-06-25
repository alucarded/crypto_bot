#include "exchange/exchange_listener.h"
#include "model/trade_ticker.h"
#include "order_book_imbalance.h"

class IndicatorsGenerator : public ExchangeListener {
public:

  IndicatorsGenerator() : m_ob_imbalance(100), m_last_ts(0) {

  }

  virtual ~IndicatorsGenerator() {

  }

  virtual void OnConnectionOpen(const std::string& name) override {

  }

  virtual void OnConnectionClose(const std::string& name) override {
    
  }

  virtual void OnBookTicker(const Ticker& ticker) override {
    BOOST_LOG_TRIVIAL(trace) << "MarketMakingStrategy::OnBookTicker, ticker=" << ticker;
    assert(ticker.arrived_ts >= m_last_ts);
    m_last_ts = ticker.arrived_ts;
  }

  virtual void OnTradeTicker(const TradeTicker& ticker) override {
    BOOST_LOG_TRIVIAL(trace) << "MarketMakingStrategy::OnTradeTicker, ticker=" << ticker;
    assert(ticker.arrived_ts >= m_last_ts);
    m_last_ts = ticker.arrived_ts;
  }

  virtual void OnOrderBookUpdate(const OrderBook& order_book) override {
    BOOST_LOG_TRIVIAL(debug) << "MarketMakingStrategy::OnOrderBookUpdate, order_book=" << order_book;
    double simple_imbalance = m_ob_imbalance.Calculate(order_book);
    BOOST_LOG_TRIVIAL(debug) << "IMBALANCE: " << simple_imbalance; 
  }

private:
  OrderBookImbalance m_ob_imbalance;
  uint64_t m_last_ts;
};