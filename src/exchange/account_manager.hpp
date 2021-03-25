#pragma once

#include "exchange_client.h"
#include "user_data_listener.hpp"

#include <atomic>
#include <memory>
#include <unordered_set>

class AccountManager : public ExchangeClient, public UserDataListener {
public:
  AccountManager(ExchangeClient* exchange_client)
      : m_client(exchange_client) {
  }

  virtual ~AccountManager() {
  }

  void Initialize() {
    auto res = m_client->GetAccountBalance();
    if (!res) {
      throw std::runtime_error("Failed getting account balance for " + GetExchange());
    }
    m_account_balance = std::move(res.Get());
    m_is_up_to_date.store(true, std::memory_order_relaxed);
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
    m_client->CancelAllOrders();
    // We should receive status update for all orders with calls to OnOrderUpdate()
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
    BOOST_LOG_TRIVIAL(info) << "Account balance: " << account_balance << std::endl;
  }

  virtual void OnOrderUpdate(const Order& order_update) override {
    BOOST_LOG_TRIVIAL(debug) << "AccountManager::OnOrderUpdate " + GetExchange();
    BOOST_LOG_TRIVIAL(debug) << "Order: " << order_update;
    auto it = m_orders.find(order_update.GetId());
    if (it == m_orders.end()) {
      BOOST_LOG_TRIVIAL(info) << "Update for order, which did not originate from current program run";
      return;
    }
    Order& order = it->second;
    switch (order.GetStatus()) {
      case OrderStatus::NEW:
        // Just add order
        break;
      case OrderStatus::PARTIALLY_FILLED:
        // Here we should update executed amount
        break;
      case OrderStatus::FILLED:
        // Update balance and remove order
        break;
      case OrderStatus::CANCELED:
        // Part of the order could have been executed
        // Update balance and remove order
        break;
      case OrderStatus::PENDING_CANCEL:
        // Do nothing ?
        break;
      case OrderStatus::REJECTED:
        // Log and remove order
        break;
      case OrderStatus::EXPIRED:
        // Log and remove order
        break;
      default:
        BOOST_LOG_TRIVIAL(error) << "Unsupported order status";
    }
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
// TODO: this class should probably not own client
  std::unique_ptr<ExchangeClient> m_client;
  std::unordered_map<std::string, Order> m_orders;
  AccountBalance m_account_balance;
  std::atomic<bool> m_is_up_to_date;
};