#include "exchange_client.h"

#include <boost/asio/io_context.hpp>
#include <boost/log/trivial.hpp>

class ArbitrageFinderClient : public ExchangeClient {
public:

  ArbitrageFinderClient(const std::string& exchange) : m_exchange(exchange) {
  }

  virtual ~ArbitrageFinderClient() {

  }

  virtual std::string GetExchange() override {
    return m_exchange;
  }

  virtual Result<Order> MarketOrder(SymbolPairId symbol, Side side, double qty) override {
    
    return Result<Order>("", Order());
  }

  virtual Result<Order> LimitOrder(SymbolPairId symbol, Side side, double qty, double price) override {
    if (m_counters.count(symbol) == 0) {
      m_counters.insert(std::make_pair(symbol, 0));
    }
    BOOST_LOG_TRIVIAL(warning) << "Arbitrage on " << symbol << ", count: " << ++m_counters[symbol];
    return Result<Order>("", Order());
  }

  virtual void CancelAllOrders() override {

  }

  virtual Result<AccountBalance> GetAccountBalance() override {
    return Result<AccountBalance>("", AccountBalance());
  }

  virtual Result<std::vector<Order>> GetOpenOrders(SymbolPairId symbol) override {
    return Result<std::vector<Order>>("", std::vector<Order>());
  }

private:
  std::string m_exchange;
  std::unordered_map<SymbolPairId, int> m_counters;
};