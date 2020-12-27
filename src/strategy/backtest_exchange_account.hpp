#include "consumer/consumer.h"
#include "exchange_account.hpp"

#include <map>
#include <unordered_map>

struct BacktestSettings {
  double m_fee;
  double m_slippage;
};

// struct CurrencyPair {
//   CurrencyPair(const std::string& symbol, const std::string& from, const std::string& to) : m_symbol(symbol), m_from(from), m_to(to) { }

//   std::string m_symbol;
//   std::string m_from;
//   std::string m_to;
// };

// std::unordered_map<std::string, CurrencyPair> PAIR_SYMBOL_MAP =
//     {
//         std::pair<std::string, CurrencyPair> ("BTCUSDT", CurrencyPair("BTCUSDT", "BTC", "USDT"))
//     };

class BacktestExchangeAccount : public ExchangeAccount {
public:
  BacktestExchangeAccount(const BacktestSettings& settings) : m_settings(settings) {
    // TODO: temporarily
    m_balances["BTC"] = 1.0;
    m_balances["USDT"] = 20000;
  }

  virtual void MarketOrder(const std::string& symbol, Side side, double qty) override {
    //std::cout << "Market order " << ((side == 1) ? "BUY" : "SELL") << std::endl;
    // TODO: for now BTCUSD (BTCUSDT) assumed
    if (side == Side::ASK) { // Selling BTC
      if (m_ticker.m_bid_vol && m_ticker.m_bid_vol.value() < qty) {
        qty = m_ticker.m_bid_vol.value();
        //std::cout << "Not enough qty for current bid price" << std::endl;
      }
      if (m_balances["BTC"] < qty) {
        //std::cout << "Not enough BTC" << std::endl;
        return;
      }
      double price = qty*(m_ticker.m_bid - m_settings.m_slippage)*(1.0 - m_settings.m_fee);
      m_balances["USDT"] = m_balances["USDT"] + price;
      m_balances["BTC"] = m_balances["BTC"] - qty;
    } else { // BID, buying BTC
      if (m_ticker.m_ask_vol && m_ticker.m_ask_vol.value() < qty) {
        qty = m_ticker.m_ask_vol.value();
        //std::cout << "Not enough qty for current ask price" << std::endl;
      }
      double price = qty*(m_ticker.m_ask + m_settings.m_slippage)*(1.0 + m_settings.m_fee);
      if (m_balances["USDT"] < price) {
        //std::cout << "Not enough USDT" << std::endl;
        return;
      }
      m_balances["USDT"] = m_balances["USDT"] - price;
      m_balances["BTC"] = m_balances["BTC"] + qty;
    }
    PrintBalances();
  }

  virtual void LimitOrder(const std::string& symbol, Side side, double qty, double price) override {
    
  }

  virtual void CancelAllOrders() override {

  }

  virtual void OnTicker(const Ticker& ticker) override {
    // Currently support only one currency pair, BTCUSD (BTCUSDT)
    m_ticker = ticker;
  }

  virtual void OnDisconnected() override {

  }

  void PrintBalances() {
    for (const std::pair<std::string, double>& b : m_balances) {
      std::cout << b.first << ": " << std::to_string(b.second) << std::endl;
    }
  }

private:
  BacktestSettings m_settings;
  std::map<std::string, double> m_balances;
  Ticker m_ticker;
};