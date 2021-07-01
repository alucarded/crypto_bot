#include "exchange/exchange_listener.h"
#include "market_making_signal.h"
#include "strategy/indicator/order_book_imbalance.h"

class MarketMakingStrategy : public ExchangeListener {
public:

  MarketMakingStrategy();

  virtual ~MarketMakingStrategy();

  virtual void OnConnectionOpen(const std::string& name) override;

  virtual void OnConnectionClose(const std::string& name) override;

  virtual void OnBookTicker(const Ticker& ticker) override;

  virtual void OnTradeTicker(const TradeTicker& ticker) override;

  virtual void OnOrderBookUpdate(const OrderBook& order_book) override;

private:
  OrderBookImbalance m_ob_imbalance;
  MarketMakingSignal m_signal;
};