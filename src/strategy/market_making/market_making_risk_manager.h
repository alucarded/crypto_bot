#include "exchange/exchange_client.h"
#include "exchange/user_data_listener.hpp"
#include "model/prediction.h"

#include <mutex>

struct MarketMakingRiskMangerOptions {
  double default_order_qty;
  double exchange_fee;
  double our_fee;
};

class MarketMakingRiskManager : public UserDataListener {
public:
  MarketMakingRiskManager(const MarketMakingRiskMangerOptions& opts, ExchangeClient* exchange_client);

  virtual ~MarketMakingRiskManager();

  virtual void OnConnectionOpen(const std::string&);

  virtual void OnConnectionClose(const std::string&);

  virtual void OnAccountBalanceUpdate(const AccountBalance&) override;

  virtual void OnOrderUpdate(const Order&) override;

  void OnPricePrediction(const MarketMakingPrediction&);

private:
  std::vector<Order> CalculateOrders(const MarketMakingPrediction&);

private:
  MarketMakingRiskMangerOptions m_options;
  ExchangeClient* m_exchange_client;
  std::vector<Order> m_orders;
  AccountBalance m_account_balance;
  // Amount of asset traded by the program.
  // Positive value means long position, negative value means short position.
  double m_trading_balance;
  uint64_t m_last_order_id;
  std::mutex m_order_mutex;
};