#include "client/exchange_client.h"
#include "consumer/consumer.h"
#include "trading_strategy.h"

#include <string>
#include <map>
#include <memory>
#include <utility>

struct ArbitrageStrategyOptions {

};

struct ExchangeParams {
  ExchangeParams(const std::string& exchange_name, double slippage, double fee)
    : m_exchange_name(exchange_name), m_slippage(slippage), m_fee(fee) {

  }

  std::string m_exchange_name;
  double m_slippage;
  double m_fee;
};

static std::map<std::string, ExchangeParams> g_exchange_params = {
  { "binance", ExchangeParams("binance", 10.0, 0.1) }
  // TODO:
};

class ArbitrageStrategy : public TradingStrategy {
public:
    ArbitrageStrategy(const ArbitrageStrategyOptions& opts, const std::map<std::string, ExchangeClient*>& exchange_clients)
      : m_opts(opts), m_exchange_clients(exchange_clients) {

  }

  virtual void execute(const std::string& updated_ticker, const std::map<std::string, Ticker>& tickers) override {
    // unused
  }

  virtual void OnTicker(const Ticker& ticker) override {
    std::cout << "Ticker." << std::endl << "Bid: " << std::to_string(ticker.m_bid) << std::endl << "Ask: " << std::to_string(ticker.m_ask) << std::endl;
    m_tickers[ticker.m_exchange] = ticker;
    auto match = FindMatch();
    if (match.has_value()) {
      // TODO:
      std::cout << "Got match: " << match.value().first << " and " <<  match.value().second << std::endl;
    }
  }

  virtual void OnDisconnected(const std::string& exchange_name) override {
    m_tickers.erase(exchange_name);
  }

  std::optional<std::pair<std::string, std::string>> FindMatch() {
    // TODO:
    return std::make_pair("", "");
  }
private:
  ArbitrageStrategyOptions m_opts;
  // TODO: perhaps add to TradingStrategy - make it a base abstract class
  std::map<std::string, ExchangeClient*> m_exchange_clients;
  std::map<std::string, Ticker> m_tickers;
};