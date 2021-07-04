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
  BOOST_LOG_TRIVIAL(debug) << "MarketMakingStrategy::OnBookTicker, ticker=" << ticker;
  m_book_ticker = ticker;
}

void MarketMakingStrategy::OnTradeTicker(const TradeTicker& ticker) {
  BOOST_LOG_TRIVIAL(debug) << "MarketMakingStrategy::OnTradeTicker, ticker=" << ticker;
}

void MarketMakingStrategy::OnOrderBookUpdate(const OrderBook& order_book) {
  BOOST_LOG_TRIVIAL(debug) << "MarketMakingStrategy::OnOrderBookUpdate, order_book=" << order_book;
  //m_ob_imbalance.Calculate(order_book, 0.01);
  // TODO: add some signal etc
  MarketMakingPrediction prediction;
  prediction.symbol = order_book.GetSymbolPairId();
  prediction.base_price = (m_book_ticker.ask + m_book_ticker.bid)/2.0d;
  prediction.signal = 0;
  prediction.confidence = 1;
  prediction.timeframe_ms = 1000000;
  m_risk_manager.OnPricePrediction(prediction);
}