#include "market_making_strategy.h"

MarketMakingStrategy::MarketMakingStrategy() : m_ob_imbalance() {

}

MarketMakingStrategy::~MarketMakingStrategy() {

}

void MarketMakingStrategy::OnConnectionOpen(const std::string&) {

}

void MarketMakingStrategy::OnConnectionClose(const std::string&) {
  
}

void MarketMakingStrategy::OnBookTicker(const Ticker& ticker) {
  BOOST_LOG_TRIVIAL(debug) << "MarketMakingStrategy::OnBookTicker, ticker=" << ticker;
}

void MarketMakingStrategy::OnTradeTicker(const TradeTicker& ticker) {
  BOOST_LOG_TRIVIAL(info) << "MarketMakingStrategy::OnTradeTicker, ticker=" << ticker;
}

void MarketMakingStrategy::OnOrderBookUpdate(const OrderBook& order_book) {
  BOOST_LOG_TRIVIAL(debug) << "MarketMakingStrategy::OnOrderBookUpdate, order_book=" << order_book;
  m_ob_imbalance.Calculate(order_book, 0.01);
}