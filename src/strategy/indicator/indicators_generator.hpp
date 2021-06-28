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
      m_weighted_price_sum(0), m_market_sell_vol(0), m_market_buy_vol(0), m_mid_price(0) {

  }

  virtual ~IndicatorsGenerator() {
     std::ofstream results_file("indicators.csv", std::ios::out | std::ios::app | std::ios::binary);
     results_file << "Weighted_Trade_Price,Mid_Price,Imbalance_0.001,Imbalance_0.01,Imbalance_0.1,Buy_Volumes,Sell_Volumes\n";
     assert(m_imbalances_1.size() == m_trade_prices.size());
     for (size_t i = 0; i < m_trade_prices.size(); ++i) {
       results_file << m_trade_prices[i] << "," << m_mid_prices[i] << "," << m_imbalances_01[i] << "," << m_imbalances_1[i] << "," << m_imbalances_10[i] << "," << m_buy_volumes[i] << "," << m_sell_volumes[i] << "\n";
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
    m_mid_price = (ticker.bid + ticker.ask)/2.0d;
    m_last_ts = ticker.arrived_ts;
  }

  // Real-time
  virtual void OnTradeTicker(const TradeTicker& ticker) override {
    BOOST_LOG_TRIVIAL(trace) << "MarketMakingStrategy::OnTradeTicker, ticker=" << ticker;
    assert(ticker.arrived_ts >= m_last_ts);
    assert(m_imbalances_01.size() == m_trade_prices.size());
    assert(m_imbalances_1.size() == m_trade_prices.size());
    assert(m_imbalances_10.size() == m_trade_prices.size());
    m_weighted_price_sum += ticker.price * ticker.qty;
    if (ticker.is_market_maker) {
      m_market_sell_vol += ticker.qty;
    } else {
      m_market_buy_vol += ticker.qty;
    }
    m_last_ts = ticker.arrived_ts;
  }

  // Updates every 1000ms or 100ms
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
    double weighted_price = m_weighted_price_sum/(m_market_sell_vol + m_market_buy_vol);
    m_trade_prices.push_back(weighted_price);
    m_imbalances_01.push_back(imbalance_01);
    m_imbalances_1.push_back(imbalance_1);
    m_imbalances_10.push_back(imbalance_10);
    m_sell_volumes.push_back(m_market_sell_vol);
    m_buy_volumes.push_back(m_market_buy_vol);
    m_mid_prices.push_back(m_mid_price);
    m_weighted_price_sum = 0;
    m_market_sell_vol = 0;
    m_market_buy_vol = 0;
    m_last_ts = ob_update.arrived_ts;
  }

private:
  OrderBookImbalance m_ob_imbalance;
  uint64_t m_last_ts;
  double m_weighted_price_sum;
  double m_market_sell_vol;
  double m_market_buy_vol;
  double m_mid_price;
  std::vector<double> m_trade_prices;
  std::vector<double> m_imbalances_01;
  std::vector<double> m_imbalances_1;
  std::vector<double> m_imbalances_10;
  std::vector<double> m_sell_volumes;
  std::vector<double> m_buy_volumes;
  std::vector<double> m_mid_prices;
};