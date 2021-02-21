#pragma once

#include "exchange_client.h"

#include <memory>

class AccountManager : public ExchangeClient {
public:
  AccountManager(ExchangeClient* exchange_client) : m_client(exchange_client) {
  }

  virtual Result<NewOrder> MarketOrder(const std::string& symbol, Side side, double qty) override {
    // TODO: register all order ids (so that we know, which ones do we manage, how many our open orders are there etc)
    return m_client->MarketOrder(symbol, side, qty);
  }

  virtual Result<NewOrder> LimitOrder(const std::string& symbol, Side side, double qty, double price) override {
    return m_client->LimitOrder(symbol, side, qty, price);
  }

  virtual Result<AccountBalance> GetAccountBalance() override {
    return m_client->GetAccountBalance();
  }

  virtual void CancelAllOrders() override {
    m_client->CancelAllOrders();
  }

  virtual Result<std::vector<Order>> GetOpenOrders() override {
    return m_client->GetOpenOrders();
  }
private:
  std::unique_ptr<ExchangeClient> m_client;
};