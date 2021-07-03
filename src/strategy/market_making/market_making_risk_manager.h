#include "exchange/exchange_client.h"
#include "exchange/user_data_listener.hpp"
#include "model/prediction.h"

struct MarketMakingRiskMangerOptions {

};

class MarketMakingRiskManager : public UserDataListener {
public:
  MarketMakingRiskManager(const MarketMakingRiskMangerOptions& opts, ExchangeClient* exchange_client);

  virtual ~MarketMakingRiskManager();

  virtual void OnConnectionOpen(const std::string&);

  virtual void OnConnectionClose(const std::string&);

  virtual void OnAccountBalanceUpdate(const AccountBalance&) override;

  virtual void OnOrderUpdate(const Order&) override;

  void OnPrediction(const RangePrediction&);

private:
  MarketMakingRiskMangerOptions m_options;
  ExchangeClient* m_exchange_client;
  std::vector<Order> m_ask_orders;
  std::vector<Order> m_bid_orders;
  AccountBalance m_account_balance;
};