#pragma once

#include "account_manager.h"
#include "account_refresher.h"
#include "exchange_client.h"
#include "user_data_listener.hpp"
#include "utils/spinlock.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_set>

class AccountManagerImpl : public AccountManager, public UserDataListener {
public:
  AccountManagerImpl(ExchangeClient* exchange_client);

  virtual ~AccountManagerImpl();

  void Initialize();

  void RefreshAccountBalance();

  // AccountManager

  virtual bool HasOpenOrders() override;

  virtual bool HasOpenOrders(SymbolPairId pair) override;

  virtual double GetFreeBalance(SymbolId symbol_id) override;

  virtual double GetTotalBalance(SymbolId symbol_id) override;

  virtual bool IsAccountSynced() const override;

  // ExchangeClient

  virtual std::string GetExchange() override;

  // This will return the cached balance
  virtual Result<AccountBalance> GetAccountBalance() override;

  virtual void CancelAllOrders() override;

  virtual Result<bool> CancelOrder(const Order& order) override;

  virtual Result<std::vector<Order>> GetOpenOrders() override;

  virtual Result<Order> MarketOrder(SymbolPairId symbol, Side side, double qty) override;

  virtual Result<Order> LimitOrder(SymbolPairId symbol, Side side, double qty, double price) override;

  virtual Result<Order> SendOrder(const Order& order) override;

  // UserDataListener

  virtual void OnConnectionOpen(const std::string& name) override;

  virtual void OnConnectionClose(const std::string& name) override;

  virtual void OnAccountBalanceUpdate(const AccountBalance& account_balance) override;

  virtual void OnOrderUpdate(const Order& order_update) override;

protected:
  // m_our_orders_lock should be locked
  void UpdateOurOrder(const Order& order_update, Order& order);

  void HandleExternalOrder(const Order& order);

  void AddLockedBalance(const Order& order);

  void SubtractLockedBalance(const Order& order);

  void AdjustLockedBalance(const Order& order, std::function<double(double, double)> operation);

  void AdjustBalanceClosedOrder(const Order& order);

protected:
  ExchangeClient* m_client;
  std::unordered_map<std::string, Order> m_external_orders;
  // TODO: re-think locks - probably there are too many
  cryptobot::spinlock m_external_orders_lock;
  std::unordered_map<std::string, Order> m_our_orders;
  cryptobot::spinlock m_our_orders_lock;
  AccountBalance m_account_balance;
  cryptobot::spinlock m_account_balance_lock;
  // The mutex prevents following race condition (at least this one):
  // 1. LimitOrder() is called and HTTP request with new order is sent
  // 2. NEW order event is received via user data stream.
  //    Since the order was not yet added to m_our_orders, it will be handled as external order.
  // 3. The order was filled immediately (as market order), so FILLED event is received
  // 3. Response from order request is received and order is added to m_our_orders.
  // 4. The balance has locked amount, which will never be freed.
  std::mutex m_order_mutex;
  AccountRefresher m_account_refresher;
  std::atomic<bool> m_is_account_synced;
};