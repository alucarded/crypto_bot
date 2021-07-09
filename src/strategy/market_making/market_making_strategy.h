#include "exchange/exchange_listener.h"
#include "market_making_signal.h"
#include "market_making_risk_manager.h"
#include "strategy/indicator/order_book_imbalance.h"

#include <mutex>

class MarketMakingStrategy : public ExchangeListener {
public:

  MarketMakingStrategy(MarketMakingRiskManager& risk_manager);

  virtual ~MarketMakingStrategy();

  virtual void OnConnectionOpen(const std::string& name) override;

  virtual void OnConnectionClose(const std::string& name) override;

  virtual void OnBookTicker(const Ticker& ticker) override;

  virtual void OnTradeTicker(const TradeTicker& ticker) override;

  virtual void OnOrderBookUpdate(const OrderBook& order_book) override;

private:
  MarketMakingRiskManager& m_risk_manager;
  OrderBookImbalance m_ob_imbalance;
  MarketMakingSignal m_signal;
  Ticker m_book_ticker;
  std::vector<double> m_mid_closes;
  std::mutex m_mutex;
};