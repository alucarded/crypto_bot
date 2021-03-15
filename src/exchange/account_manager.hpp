#pragma once

#include "exchange_client.h"
#include "user_data_listener.hpp"

#include <memory>
#include <unordered_set>

class AccountManager : public ExchangeClient, public UserDataListener {
public:
  AccountManager(ExchangeClient* exchange_client)
      : m_client(exchange_client) {
  }

  virtual ~AccountManager() {
  }

  virtual std::string GetExchange() override {
    return m_client->GetExchange();
  }

  // ExchangeClient

  virtual Result<Order> MarketOrder(SymbolPairId symbol, Side side, double qty) override {
    // TODO: register all order ids (so that we know, which ones do we manage, how many our open orders are there etc)
    auto res = m_client->MarketOrder(symbol, side, qty);
    if (res) {
      m_orders.insert(std::make_pair(res.Get().GetId(), res.Get()));
    }
    return res;
  }

  virtual Result<Order> LimitOrder(SymbolPairId symbol, Side side, double qty, double price) override {
    auto res = m_client->LimitOrder(symbol, side, qty, price);
    if (res) {
      m_orders.insert(std::make_pair(res.Get().GetId(), res.Get()));
    }
    return res;
  }

  virtual Result<AccountBalance> GetAccountBalance() override {
    return m_client->GetAccountBalance();
  }

  virtual void CancelAllOrders() override {
    // TODO: we should make sure cancellation succeeded
    m_orders.clear();
    m_client->CancelAllOrders();
  }

  virtual Result<std::vector<Order>> GetOpenOrders(SymbolPairId symbol) override {
    return m_client->GetOpenOrders(symbol);
  }

  // UserDataListener

  virtual void OnConnectionOpen(const std::string& name) override {
    assert(name == GetExchange());
    BOOST_LOG_TRIVIAL(info) << "AccountManager::OnConnectionOpen " + GetExchange();
  }

  virtual void OnConnectionClose(const std::string& name) override {
    assert(name == GetExchange());
    BOOST_LOG_TRIVIAL(info) << "AccountManager::OnConnectionClose " + GetExchange();
  }

  virtual void OnAccountBalanceUpdate(const AccountBalance& account_balance) override {
    BOOST_LOG_TRIVIAL(info) << "AccountManager::OnAccountBalanceUpdate " + GetExchange();
    BOOST_LOG_TRIVIAL(info) << account_balance << std::endl;
  }

  virtual void OnOrderUpdate(const Order& order) override {
    BOOST_LOG_TRIVIAL(info) << "AccountManager::OnOrderUpdate " + GetExchange();
    BOOST_LOG_TRIVIAL(info) << order << std::endl;
  }

  virtual bool HasOpenOrders(SymbolPairId symbol) {
    // TODO: order execution should come from websocket
    // auto res = GetOpenOrders(symbol);
    // if (!res) {
    //   BOOST_LOG_TRIVIAL(warning) << "Could not get open orders for " << GetExchange() << std::endl;
    //   return true;
    // }
    // for (const auto& o : res.Get()) {
    //   if (m_orders.count(o.GetId()) > 0) {
    //     return true;
    //   }
    // }
    return false;
  }

protected:
  std::unique_ptr<ExchangeClient> m_client;
  std::unordered_map<std::string, Order> m_orders;
};