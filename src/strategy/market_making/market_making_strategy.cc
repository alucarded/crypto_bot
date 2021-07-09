#include "market_making_strategy.h"

MarketMakingStrategy::MarketMakingStrategy(MarketMakingRiskManager& risk_manager) : m_risk_manager(risk_manager), m_ob_imbalance() {

}

MarketMakingStrategy::~MarketMakingStrategy() {

}

void MarketMakingStrategy::OnConnectionOpen(const std::string&) {

}

void MarketMakingStrategy::OnConnectionClose(const std::string&) {
  
}

void MarketMakingStrategy::OnBookTicker(const Ticker& ticker) {
  std::scoped_lock<std::mutex> lock{m_mutex};
  BOOST_LOG_TRIVIAL(debug) << "MarketMakingStrategy::OnBookTicker, ticker=" << ticker;
  m_book_ticker = ticker;
  m_signal.OnBookTicker(ticker);
}

void MarketMakingStrategy::OnTradeTicker(const TradeTicker& ticker) {
  std::scoped_lock<std::mutex> lock{m_mutex};
  BOOST_LOG_TRIVIAL(debug) << "MarketMakingStrategy::OnTradeTicker, ticker=" << ticker;
  m_signal.OnTradeTicker(ticker);
}

void MarketMakingStrategy::OnOrderBookUpdate(const OrderBook& order_book) {
  std::scoped_lock<std::mutex> lock{m_mutex};
  BOOST_LOG_TRIVIAL(debug) << "MarketMakingStrategy::OnOrderBookUpdate, order_book=" << order_book;
  //m_ob_imbalance.Calculate(order_book, 0.01);
  m_signal.OnOrderBookUpdate(order_book);

  MarketMakingPredictionData data;
  MarketMakingPrediction prediction = m_signal.Predict(data);
  m_risk_manager.OnPricePrediction(prediction);
}