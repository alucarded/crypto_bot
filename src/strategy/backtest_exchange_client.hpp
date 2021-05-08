#include "exchange/exchange_client.h"
#include "exchange/exchange_listener.h"

#include <fstream>
#include <map>
#include <unordered_map>

struct BacktestSettings {
  std::string m_exchange;
  double m_fee;
  double m_slippage;
  double m_min_qty;
};

class BacktestExchangeClient : public ExchangeClient, public ExchangeListener {
public:
  BacktestExchangeClient(const BacktestSettings& settings, const std::string& results_file) : m_settings(settings),
      m_results_file(results_file, std::ios::out | std::ios::app | std::ios::binary) {
    // TODO: temporarily
    m_balances[SymbolId::BTC] = 1.0;
    m_balances[SymbolId::USDT] = 24000;
    m_results_file << "BTC,USDT,Direction,Quantity,Rate,Price\n";
  }

  virtual ~BacktestExchangeClient() {
    m_results_file.close();
  }

  virtual std::string GetExchange() override {
    return "backtest";
  }

  virtual Result<Order> MarketOrder(SymbolPairId symbol, Side side, double qty) override {
    //std::cout << "Market order " << ((side == 1) ? "BUY" : "SELL") << std::endl;
    // TODO: for now BTCUSD (BTCUSDT) assumed, add enum for symbols, data types (tickers, order book etc ?) and exchanges
    double rate, price;
    if (side == Side::SELL) { // Selling BTC
      rate = m_ticker.m_bid;
      if (m_ticker.m_bid_vol && m_ticker.m_bid_vol.value() < qty) {
        qty = m_ticker.m_bid_vol.value();
        //std::cout << "Not enough qty for current bid price" << std::endl;
      }
      if (m_balances.at(SymbolId::BTC) < qty) {
        std::cout << "Not enough BTC" << std::endl;
        return Result<Order>("", "");
      }
      price = qty*(m_ticker.m_bid - m_settings.m_slippage)*(1.0 - m_settings.m_fee);
      m_balances[SymbolId::USDT] = m_balances.at(SymbolId::USDT) + price;
      m_balances[SymbolId::BTC] = m_balances.at(SymbolId::BTC) - qty;
    } else { // Buying BTC
      rate = m_ticker.m_ask;
      if (m_ticker.m_ask_vol && m_ticker.m_ask_vol.value() < qty) {
        qty = m_ticker.m_ask_vol.value();
        //std::cout << "Not enough qty for current ask price" << std::endl;
      }
      price = qty*(m_ticker.m_ask + m_settings.m_slippage)*(1.0 + m_settings.m_fee);
      if (m_balances[SymbolId::USDT] < price) {
        std::cout << "Not enough USDT" << std::endl;
        return Result<Order>("", "");
      }
      m_balances[SymbolId::USDT] = m_balances[SymbolId::USDT] - price;
      m_balances[SymbolId::BTC] = m_balances[SymbolId::BTC] + qty;
    }
    PrintBalances(side, qty, rate, price);
    return Result<Order>("", "");
  }

  virtual Result<Order> LimitOrder(SymbolPairId symbol, Side side, double qty, double price) override {
    return Result<Order>("", "");
  }

  virtual Result<AccountBalance> GetAccountBalance() override {
    return Result<AccountBalance>("", AccountBalance());
  }

  virtual Result<std::vector<Order>> GetOpenOrders() override {
    return Result<std::vector<Order>>("", std::vector<Order>());
  }

  virtual void CancelAllOrders() override {

  }

  virtual void OnBookTicker(const Ticker& ticker) override {
    // Currently support only one currency pair, BTCUSD (BTCUSDT)
    if (m_settings.m_exchange == ticker.m_exchange) {
      m_ticker = ticker;
    }
  }

  void PrintBalances(Side side, double qty, double rate, double price) {
    m_results_file << std::to_string(m_balances[SymbolId::BTC]) << "," << std::to_string(m_balances[SymbolId::USDT])
        << "," << (side == Side::SELL ? "SELL" : "BUY") << "," << std::to_string(qty)
        << "," << std::to_string(rate) << "," << std::to_string(price) << "\n";
    m_results_file.flush();
  }

private:
  BacktestSettings m_settings;
  std::map<SymbolId, double> m_balances;
  Ticker m_ticker;
  std::ofstream m_results_file;
};