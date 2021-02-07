//#include "client/exchange_client.h"
#include "consumer/consumer.h"
#include "ticker_subscriber.h"

#include <fstream>
#include <map>
#include <unordered_map>

struct BacktestSettings {
  std::string m_exchange;
  double m_fee;
  double m_slippage;
  double m_min_qty;
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

class BacktestExchangeClient : public ExchangeClient, public TickerSubscriber {
public:
  BacktestExchangeClient(const BacktestSettings& settings, const std::string& results_file) : m_settings(settings),
      m_results_file(results_file, std::ios::out | std::ios::app | std::ios::binary) {
    // TODO: temporarily
    m_balances["BTC"] = 1.0;
    m_balances["USDT"] = 24000;
    m_results_file << "BTC,USDT,Direction,Quantity,Rate,Price\n";
  }

  virtual ~BacktestExchangeClient() {
    m_results_file.close();
  }

  virtual void MarketOrder(const std::string& symbol, Side side, double qty) override {
    //std::cout << "Market order " << ((side == 1) ? "BUY" : "SELL") << std::endl;
    // TODO: for now BTCUSD (BTCUSDT) assumed, add enum for symbols, data types (tickers, order book etc ?) and exchanges
    double rate, price;
    if (side == Side::ASK) { // Selling BTC
      rate = m_ticker.m_bid;
      if (m_ticker.m_bid_vol && m_ticker.m_bid_vol.value() < qty) {
        qty = m_ticker.m_bid_vol.value();
        //std::cout << "Not enough qty for current bid price" << std::endl;
      }
      if (m_balances["BTC"] < qty) {
        std::cout << "Not enough BTC" << std::endl;
        return;
      }
      price = qty*(m_ticker.m_bid - m_settings.m_slippage)*(1.0 - m_settings.m_fee);
      m_balances["USDT"] = m_balances["USDT"] + price;
      m_balances["BTC"] = m_balances["BTC"] - qty;
    } else { // BID, buying BTC
      rate = m_ticker.m_ask;
      if (m_ticker.m_ask_vol && m_ticker.m_ask_vol.value() < qty) {
        qty = m_ticker.m_ask_vol.value();
        //std::cout << "Not enough qty for current ask price" << std::endl;
      }
      price = qty*(m_ticker.m_ask + m_settings.m_slippage)*(1.0 + m_settings.m_fee);
      if (m_balances["USDT"] < price) {
        std::cout << "Not enough USDT" << std::endl;
        return;
      }
      m_balances["USDT"] = m_balances["USDT"] - price;
      m_balances["BTC"] = m_balances["BTC"] + qty;
    }
    PrintBalances(side, qty, rate, price);
  }

  virtual void LimitOrder(const std::string& symbol, Side side, double qty, double price) override {
    
  }

  virtual void CancelAllOrders() override {

  }

  virtual void OnTicker(const Ticker& ticker) override {
    // Currently support only one currency pair, BTCUSD (BTCUSDT)
    if (m_settings.m_exchange == ticker.m_exchange) {
      m_ticker = ticker;
    }
  }

  virtual void OnDisconnected(const std::string&) override {

  }

  void PrintBalances(Side side, double qty, double rate, double price) {
    m_results_file << std::to_string(m_balances["BTC"]) << "," << std::to_string(m_balances["USDT"])
        << "," << (side == Side::ASK ? "SELL" : "BUY") << "," << std::to_string(qty)
        << "," << std::to_string(rate) << "," << std::to_string(price) << "\n";
    m_results_file.flush();
  }

private:
  BacktestSettings m_settings;
  std::map<std::string, double> m_balances;
  Ticker m_ticker;
  std::ofstream m_results_file;
};