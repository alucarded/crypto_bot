#include "exchange/exchange_listener.h"
#include "model/trade_ticker.h"

class MarketMakingStrategy : public ExchangeListener {
public:

  virtual ~MarketMakingStrategy() {
    
  }

  virtual void OnConnectionOpen(const std::string& name) override {

  }

  virtual void OnConnectionClose(const std::string& name) override {
    
  }

  virtual void OnTradeTicker(const TradeTicker& ticker) override {
    BOOST_LOG_TRIVIAL(info) << "ExchangeListener::OnTradeTicker, ticker=" << ticker;
  }
};