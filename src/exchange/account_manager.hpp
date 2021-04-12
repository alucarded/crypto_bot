#pragma once

#include "exchange_client.h"
#include "user_data_listener.hpp"
#include "utils/spinlock.hpp"

#include <atomic>
#include <functional>
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
    m_account_balance_lock.lock();
    m_account_balance = std::move(res.Get());
    m_account_balance_lock.unlock();
    //m_is_up_to_date.store(true, std::memory_order_relaxed);

    auto open_orders_res = m_client->GetOpenOrders();
    if (!open_orders_res) {
      throw std::runtime_error("Failed getting orders for " + GetExchange());
    }
    m_external_orders_lock.lock();
    m_external_orders.clear();
    m_external_orders_lock.unlock();
    auto& orders = open_orders_res.Get();
    for (auto& o : orders) {
      if (o.GetSymbolId() == SymbolPairId::UNKNOWN) {
        BOOST_LOG_TRIVIAL(error) << "Order for unknown pair, exiting";
        std::exit(1);
      }
      HandleExternalOrder(o);
    }
  }

  virtual std::string GetExchange() override {
    return m_client->GetExchange();
  }

  // ExchangeClient

  virtual Result<Order> MarketOrder(SymbolPairId symbol, Side side, double qty) override {
    // TODO: register all order ids (so that we know, which ones do we manage, how many our open orders are there etc)
    auto res = m_client->MarketOrder(symbol, side, qty);
    if (res) {
      auto& order = res.Get();
      m_our_orders.insert_or_assign(order.GetId(), std::move(order));
      AddLockedBalance(order);
      BOOST_LOG_TRIVIAL(trace) << "Account balance after placing market order: " << m_account_balance;
    }
    return res;
  }

  virtual Result<Order> LimitOrder(SymbolPairId symbol, Side side, double qty, double price) override {
    auto res = m_client->LimitOrder(symbol, side, qty, price);
    if (res) {
      auto& order = res.Get();
      order.SetPrice(price);
      m_our_orders.insert_or_assign(order.GetId(), std::move(order));
      AddLockedBalance(order);
      BOOST_LOG_TRIVIAL(trace) << "Account balance after placing limit order: " << m_account_balance;
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

  virtual Result<std::vector<Order>> GetOpenOrders() override {
    return m_client->GetOpenOrders();
  }

  // UserDataListener

  virtual void OnConnectionOpen(const std::string& name) override {
    assert(name == GetExchange());
    BOOST_LOG_TRIVIAL(info) << "AccountManager::OnConnectionOpen " + GetExchange();
    Initialize();
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
    if (order_update.GetSymbolId() == SymbolPairId::UNKNOWN) {
      BOOST_LOG_TRIVIAL(error) << "Order for unknown pair, exiting";
      std::exit(1);
    }
    m_our_orders_lock.lock();
    auto it = m_our_orders.find(order_update.GetId());
    if (it == m_our_orders.end()) {
      m_our_orders_lock.unlock();
      BOOST_LOG_TRIVIAL(info) << "Update for order, which did not originate from here";
      HandleExternalOrder(order_update);
      BOOST_LOG_TRIVIAL(debug) << "Account balance after order update: " << m_account_balance;
      return;
    }
    Order& order = it->second;
    BOOST_LOG_TRIVIAL(trace) << "AccountManager::UpdateOurOrder";
    UpdateOurOrder(order_update, order);
    BOOST_LOG_TRIVIAL(debug) << "Account balance after order update: " << m_account_balance;
    m_our_orders_lock.unlock();
  }

  bool HasOpenOrders() {
    m_our_orders_lock.lock();
    bool ret = !m_our_orders.empty();
    m_our_orders_lock.unlock();
    return ret;
  }

  bool HasOpenOrders(SymbolPairId pair) {
    m_our_orders_lock.lock();
    for (auto it = m_our_orders.cbegin(); it != m_our_orders.cend(); ++it) {
      if (it->second.GetSymbolId() == pair) {
        return true;
      }
    }
    m_our_orders_lock.unlock();
    return false;
  }

  double GetFreeBalance(SymbolId symbol_id) {
    m_account_balance_lock.lock();
    double ret = m_account_balance.GetFreeBalance(symbol_id);
    m_account_balance_lock.unlock();
    return ret;
  }

  double GetTotalBalance(SymbolId symbol_id) {
    m_account_balance_lock.lock();
    double ret = m_account_balance.GetTotalBalance(symbol_id);
    m_account_balance_lock.unlock();
    return ret;
  }
protected:
  // m_our_orders_lock should be locked
  void UpdateOurOrder(const Order& order_update, Order& order) {

    order.SetStatus(order_update.GetStatus());

    switch (order_update.GetStatus()) {
      case OrderStatus::NEW:
        // Replace order with updated one
        order = order_update;
        break;
      case OrderStatus::PARTIALLY_FILLED:
        // Here we should update executed amount
        order.SetExecutedQuantity(order_update.GetExecutedQuantity());
        // TODO: here we could adjust total balance
        break;
      case OrderStatus::FILLED:
        order.SetExecutedQuantity(order.GetQuantity());
        // TODO: take into account fee and fee currency
        order.SetTotalCost(order.GetQuantity() * order.GetPrice());
        AdjustBalanceClosedOrder(order);
        m_our_orders.erase(order.GetId());
        break;
      case OrderStatus::CANCELED:
        AdjustBalanceClosedOrder(order);
        m_our_orders.erase(order.GetId());
        break;
      case OrderStatus::PENDING_CANCEL:
        // Do nothing ?
        BOOST_LOG_TRIVIAL(info) << "Order pending cancel " << order;
        break;
      case OrderStatus::REJECTED:
        BOOST_LOG_TRIVIAL(error) << "Unexpected order status for " << order;
        break;
      case OrderStatus::EXPIRED:
        // Log and remove order
        BOOST_LOG_TRIVIAL(info) << "Order expired " << order;
        SubtractLockedBalance(order);
        m_our_orders.erase(order.GetId());
        break;
      default:
        BOOST_LOG_TRIVIAL(error) << "Unsupported order status";
    }
  }

  void HandleExternalOrder(const Order& order) {
    switch (order.GetStatus()) {
        case OrderStatus::NEW: {
          m_external_orders_lock.lock();
          if (m_external_orders.count(order.GetId()) > 0) {
            m_external_orders_lock.unlock();
            BOOST_LOG_TRIVIAL(warning) << "Order already added: " << order.GetId();
            return;
          }
          m_external_orders.emplace(order.GetId(), order);
          m_external_orders_lock.unlock();

          AddLockedBalance(order);
        }
        break;
      case OrderStatus::PENDING_CANCEL:
      case OrderStatus::PARTIALLY_FILLED:
        // Ignore
        break;
      case OrderStatus::FILLED:
      case OrderStatus::CANCELED:
      case OrderStatus::REJECTED:
      case OrderStatus::EXPIRED: {
          //TODO:
          m_external_orders_lock.lock();
          auto it = m_external_orders.find(order.GetId());
          if (it == m_external_orders.end()) {
            m_external_orders_lock.unlock();
            BOOST_LOG_TRIVIAL(warning) << "Order not present: " << order.GetId();
            return;
          }
          it->second.SetStatus(order.GetStatus());
          AdjustBalanceClosedOrder(it->second);
          m_external_orders.erase(it);
          m_external_orders_lock.unlock();
        }
        break;
      default:
        BOOST_LOG_TRIVIAL(error) << "Unsupported order status";
        std::exit(1);
    }
  }

  void AddLockedBalance(const Order& order) {
    AdjustLockedBalance(order, std::plus<>{});
  }

  void SubtractLockedBalance(const Order& order) {
    AdjustLockedBalance(order, std::minus<>{});
  }

  void AdjustLockedBalance(const Order& order, std::function<double(double, double)> operation) {
    double qty = order.GetQuantity();
    SymbolPair symbol_pair{order.GetSymbolId()};
    Side side = order.GetSide();
    if (side == Side::BUY) {
      SymbolId quote_asset = symbol_pair.GetQuoteAsset();
      // TODO: make sure we always have price here
      double quote_qty = qty * order.GetPrice();
      m_account_balance_lock.lock();
      m_account_balance.SetLockedBalance(quote_asset, operation(m_account_balance.GetLockedBalance(quote_asset), quote_qty));
      m_account_balance_lock.unlock();
    } else { // Side::SELL
      SymbolId base_asset = symbol_pair.GetBaseAsset();
      m_account_balance_lock.lock();
      m_account_balance.SetLockedBalance(base_asset, operation(m_account_balance.GetLockedBalance(base_asset), qty));
      m_account_balance_lock.unlock();
    }
  }

  void AdjustBalanceClosedOrder(const Order& order) {
    BOOST_LOG_TRIVIAL(debug) << "AdjustBalanceClosedOrder, order: " << order;
    SubtractLockedBalance(order);
    // Adjust balance based on executed amount
    auto executed_qty = order.GetExecutedQuantity();
    // TODO: make sure fee is taken into account here
    auto executed_cost = order.GetTotalCost();
    SymbolPair symbol_pair{order.GetSymbolId()};
    SymbolId base_asset = symbol_pair.GetBaseAsset();
    SymbolId quote_asset = symbol_pair.GetQuoteAsset();
    Side side = order.GetSide();
    // TODO: make sure this logic is fine for all exchanges
    if (side == Side::BUY) {
      m_account_balance_lock.lock();
      m_account_balance.SetTotalBalance(base_asset, m_account_balance.GetTotalBalance(base_asset) + executed_qty);
      m_account_balance.SetTotalBalance(quote_asset, m_account_balance.GetTotalBalance(quote_asset) - executed_cost);
      m_account_balance_lock.unlock();
    } else { // Side::SELL
      m_account_balance_lock.lock();
      m_account_balance.SetTotalBalance(base_asset, m_account_balance.GetTotalBalance(base_asset) - executed_qty);
      m_account_balance.SetTotalBalance(quote_asset, m_account_balance.GetTotalBalance(quote_asset) + executed_cost);
      m_account_balance_lock.unlock();
    }
  }

protected:
  ExchangeClient* m_client;
  std::unordered_map<std::string, Order> m_external_orders;
  cryptobot::spinlock m_external_orders_lock;
  std::unordered_map<std::string, Order> m_our_orders;
  cryptobot::spinlock m_our_orders_lock;
  AccountBalance m_account_balance;
  cryptobot::spinlock m_account_balance_lock;
};