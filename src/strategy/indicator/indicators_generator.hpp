#include "exchange/exchange_listener.h"
#include "model/trade_ticker.h"
#include "order_book_imbalance.h"

#include <chrono>
#include <fstream>
#include <iostream>

using namespace std::chrono;

class IndicatorsGenerator : public ExchangeListener {
public:

  IndicatorsGenerator() : m_ob_imbalance(), m_last_ts(0),
      m_current_second(0), m_last_trade_price(0), m_last_imbalance_01(0), m_last_imbalance_1(0), m_last_imbalance_10(0) {

  }

  virtual ~IndicatorsGenerator() {
     std::ofstream results_file("indicators.csv", std::ios::out | std::ios::app | std::ios::binary);
     results_file << "Price,Imbalance_0.001,Imbalance_0.01,Imbalance_0.1\n";
     assert(m_imbalances_1.size() == m_trade_prices.size());
     for (size_t i = 0; i < m_trade_prices.size(); ++i) {
       results_file << m_trade_prices[i] << "," << m_imbalances_01[i] << "," << m_imbalances_1[i] << "," << m_imbalances_10[i] << "\n";
     }
     results_file.close();
  }

  virtual void OnConnectionOpen(const std::string& name) override {

  }

  virtual void OnConnectionClose(const std::string& name) override {
    
  }

  virtual void OnBookTicker(const Ticker& ticker) override {
    BOOST_LOG_TRIVIAL(trace) << "MarketMakingStrategy::OnBookTicker, ticker=" << ticker;
    assert(ticker.arrived_ts >= m_last_ts);
    assert(m_imbalances_1.size() == m_trade_prices.size());
    m_last_ts = ticker.arrived_ts;
  }

  virtual void OnTradeTicker(const TradeTicker& ticker) override {
    BOOST_LOG_TRIVIAL(trace) << "MarketMakingStrategy::OnTradeTicker, ticker=" << ticker;
    assert(ticker.arrived_ts >= m_last_ts);
    assert(m_imbalances_01.size() == m_trade_prices.size());
    assert(m_imbalances_1.size() == m_trade_prices.size());
    assert(m_imbalances_10.size() == m_trade_prices.size());
    HandleTick(ticker.arrived_ts);
    m_last_trade_price = ticker.price;
    m_last_ts = ticker.arrived_ts;

  }

  virtual void OnOrderBookUpdate(const OrderBook& order_book) override {
    BOOST_LOG_TRIVIAL(trace) << "MarketMakingStrategy::OnOrderBookUpdate, order_book=" << order_book;
    double imbalance_01 = m_ob_imbalance.Calculate(order_book, 0.001);
    double imbalance_1 = m_ob_imbalance.Calculate(order_book, 0.01);
    double imbalance_10 = m_ob_imbalance.Calculate(order_book, 0.10);
    const OrderBookUpdate& ob_update = order_book.GetLastUpdate();
    assert(ob_update.arrived_ts >= m_last_ts);
    assert(m_imbalances_01.size() == m_trade_prices.size());
    assert(m_imbalances_1.size() == m_trade_prices.size());
    assert(m_imbalances_10.size() == m_trade_prices.size());
    HandleTick(ob_update.arrived_ts);
    m_last_imbalance_01 = imbalance_01;
    m_last_imbalance_1 = imbalance_1;
    m_last_imbalance_10 = imbalance_10;
    m_last_ts = ob_update.arrived_ts;
  }

private:
  void HandleTick(uint64_t arrived_ts) {
    microseconds us = microseconds(arrived_ts);
    seconds secs = duration_cast<seconds>(us);
    if (secs > m_current_second) {
      m_trade_prices.push_back(m_last_trade_price);
      m_imbalances_01.push_back(m_last_imbalance_01);
      m_imbalances_1.push_back(m_last_imbalance_1);
      m_imbalances_10.push_back(m_last_imbalance_10);
    }
    m_current_second = secs;
  }

private:
  OrderBookImbalance m_ob_imbalance;
  uint64_t m_last_ts;
  seconds m_current_second;
  double m_last_trade_price;
  double m_last_imbalance_01;
  double m_last_imbalance_1;
  double m_last_imbalance_10;
  std::vector<double> m_trade_prices;
  std::vector<double> m_imbalances_01;
  std::vector<double> m_imbalances_1;
  std::vector<double> m_imbalances_10;
};