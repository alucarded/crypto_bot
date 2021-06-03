#pragma once

#include "account_manager_impl.h"
#include "account_refresher.h"
#include "exchange_client.h"
#include "user_data_listener.hpp"
#include "utils/spinlock.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_set>

AccountManagerImpl::AccountManagerImpl(ExchangeClient* exchange_client)
    : m_client(exchange_client), m_account_refresher(this), m_is_account_synced(false) {
}

AccountManagerImpl::~AccountManagerImpl() {
}

void AccountManagerImpl::Initialize() {
  std::scoped_lock<std::mutex> order_lock{m_order_mutex};
  m_account_refresher.Start();

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
      BOOST_LOG_TRIVIAL(warning) << "Order for unknown pair!";
      continue;
    }
    HandleExternalOrder(o);
  }
}

void AccountManagerImpl::RefreshAccountBalance() {
  // TODO: FIXME: implement asynchronous pooled http client and avoid locking so much!
  std::scoped_lock<std::mutex> order_lock{m_order_mutex};
  auto res = m_client->GetAccountBalance();
  if (!res) {
    BOOST_LOG_TRIVIAL(warning) << "Failed getting account balance for " + GetExchange();
    m_is_account_synced = false;
  }
  m_account_balance_lock.lock();
  m_account_balance = std::move(res.Get());
  m_account_balance_lock.unlock();
  // Replay cached orders
  m_external_orders_lock.lock();
  for (const auto& it: m_external_orders) {
    AddLockedBalance(it.second);
  }
  m_external_orders_lock.unlock();
  m_our_orders_lock.lock();
  for (const auto& it: m_our_orders) {
    AddLockedBalance(it.second);
  }
  m_our_orders_lock.unlock();
  m_account_balance_lock.lock();
  BOOST_LOG_TRIVIAL(debug) << GetExchange() + " - account balance after refresh: " << m_account_balance;
  m_account_balance_lock.unlock();
  m_is_account_synced = true;
}

std::string AccountManagerImpl::GetExchange() {
  return m_client->GetExchange();
}

// ExchangeClient

Result<Order> AccountManagerImpl::MarketOrder(SymbolPairId symbol, Side side, double qty) {
  std::scoped_lock<std::mutex> order_lock{m_order_mutex};
  // TODO: register all order ids (so that we know, which ones do we manage, how many our open orders are there etc)
  // TODO: pass expected price as not all exchanges will respond with filled quote amount or price (eg. Kraken)
  // Account is not synced until we got a response (we do not know if and when exactly it will be placed)
  m_is_account_synced = false;
  auto res = m_client->MarketOrder(symbol, side, qty);
  if (res) {
    auto& order = res.Get();
    m_our_orders_lock.lock();
    m_our_orders.insert_or_assign(order.GetId(), std::move(order));
    m_our_orders_lock.unlock();
    AddLockedBalance(order);
    BOOST_LOG_TRIVIAL(debug) << "Account balance after placing market order: " << m_account_balance;
  }
  m_is_account_synced = true;
  return res;
}

Result<Order> AccountManagerImpl::LimitOrder(SymbolPairId symbol, Side side, double qty, double price) {
  std::scoped_lock<std::mutex> order_lock{m_order_mutex};
  m_is_account_synced = false;
  auto res = m_client->LimitOrder(symbol, side, qty, price);
  if (res) {
    auto& order = res.Get();
    order.SetPrice(price);
    m_our_orders_lock.lock();
    m_our_orders.insert_or_assign(order.GetId(), std::move(order));
    m_our_orders_lock.unlock();
    AddLockedBalance(order);
    BOOST_LOG_TRIVIAL(debug) << "Account balance after placing limit order: " << m_account_balance;
  }
  m_is_account_synced = true;
  return res;
}

// This will return the cached balance
Result<AccountBalance> AccountManagerImpl::GetAccountBalance() {
  return Result<AccountBalance>("", m_account_balance);
  //return m_client->GetAccountBalance();
}

void AccountManagerImpl::CancelAllOrders() {
  m_client->CancelAllOrders();
  // We should receive status update for all orders with calls to OnOrderUpdate()
}

Result<std::vector<Order>> AccountManagerImpl::GetOpenOrders() {
  return m_client->GetOpenOrders();
}

// UserDataListener

void AccountManagerImpl::OnConnectionOpen(const std::string& name) {
  assert(name == GetExchange());
  BOOST_LOG_TRIVIAL(info) << "AccountManagerImpl::OnConnectionOpen " + GetExchange();
  Initialize();
}

void AccountManagerImpl::OnConnectionClose(const std::string& name) {
  assert(name == GetExchange());
  BOOST_LOG_TRIVIAL(info) << "AccountManagerImpl::OnConnectionClose " + GetExchange();
}

void AccountManagerImpl::OnAccountBalanceUpdate(const AccountBalance& account_balance) {
  std::scoped_lock<std::mutex> order_lock{m_order_mutex};
  BOOST_LOG_TRIVIAL(info) << "AccountManagerImpl::OnAccountBalanceUpdate " + GetExchange();
  BOOST_LOG_TRIVIAL(info) << "Account balance: " << account_balance << std::endl;
  m_account_balance_lock.lock();
  m_account_balance.UpdateTotalBalance(account_balance);
  m_account_balance_lock.unlock();
}

void AccountManagerImpl::OnOrderUpdate(const Order& order_update) {
  BOOST_LOG_TRIVIAL(debug) << "AccountManagerImpl::OnOrderUpdate " + GetExchange();
  BOOST_LOG_TRIVIAL(debug) << "Order: " << order_update;
  if (order_update.GetSymbolId() == SymbolPairId::UNKNOWN) {
      BOOST_LOG_TRIVIAL(warning) << "Order for unknown pair!";
      return;
  }
  {
    std::scoped_lock<std::mutex> order_lock{m_order_mutex};
    // TODO: remove m_our_orders_lock ?
    m_our_orders_lock.lock();
    auto it_our = m_our_orders.find(order_update.GetId());
    bool is_our = it_our != m_our_orders.end();
    m_our_orders_lock.unlock();
    if (!is_our) {
      BOOST_LOG_TRIVIAL(info) << "Update for order, which did not originate from here";
      HandleExternalOrder(order_update);
    } else {
      m_our_orders_lock.lock();
      Order& order = it_our->second;
      BOOST_LOG_TRIVIAL(trace) << "AccountManagerImpl::UpdateOurOrder";
      UpdateOurOrder(order_update, order);
      m_our_orders_lock.unlock();
    }
    m_account_balance_lock.lock();
    BOOST_LOG_TRIVIAL(debug) << "Account balance after order update: " << m_account_balance;
    m_account_balance_lock.unlock();
  }
}

bool AccountManagerImpl::HasOpenOrders() {
  m_our_orders_lock.lock();
  bool ret = !m_our_orders.empty();
  m_our_orders_lock.unlock();
  return ret;
}

bool AccountManagerImpl::HasOpenOrders(SymbolPairId pair) {
  m_our_orders_lock.lock();
  for (auto it = m_our_orders.cbegin(); it != m_our_orders.cend(); ++it) {
    if (it->second.GetSymbolId() == pair) {
      m_our_orders_lock.unlock();
      return true;
    }
  }
  m_our_orders_lock.unlock();
  return false;
}

double AccountManagerImpl::GetFreeBalance(SymbolId symbol_id) {
  m_account_balance_lock.lock();
  double ret = m_account_balance.GetFreeBalance(symbol_id);
  m_account_balance_lock.unlock();
  BOOST_LOG_TRIVIAL(debug) << "Free " << symbol_id << " balance: " << ret;
  return ret;
}

double AccountManagerImpl::GetTotalBalance(SymbolId symbol_id) {
  m_account_balance_lock.lock();
  double ret = m_account_balance.GetTotalBalance(symbol_id);
  m_account_balance_lock.unlock();
  return ret;
}

bool AccountManagerImpl::IsAccountSynced() const {
  return m_is_account_synced;
}

// m_our_orders_lock should be locked
void AccountManagerImpl::UpdateOurOrder(const Order& order_update, Order& order) {

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
      SubtractLockedBalance(order);
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

void AccountManagerImpl::HandleExternalOrder(const Order& order) {
  m_external_orders_lock.lock();
  auto it = m_external_orders.find(order.GetId());
  bool was_added = it != m_external_orders.end();
  m_external_orders_lock.unlock();
  switch (order.GetStatus()) {
      case OrderStatus::NEW: {
        if (was_added) {
          BOOST_LOG_TRIVIAL(warning) << "Order already added: " << order.GetId();
          return;
        }
        m_external_orders_lock.lock();
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
        if (!was_added) {
          BOOST_LOG_TRIVIAL(warning) << "Order not present: " << order.GetId();
          return;
        }
        m_external_orders_lock.lock();
        it->second.SetExecutedQuantity(order.GetQuantity());
        // TODO: take into account fee and fee currency
        it->second.SetTotalCost(order.GetQuantity() * order.GetPrice());
        it->second.SetStatus(order.GetStatus());
        AdjustBalanceClosedOrder(it->second);
        m_external_orders.erase(it);
        m_external_orders_lock.unlock();
      }
      break;
    default:
      BOOST_LOG_TRIVIAL(error) << "Unsupported order status";
  }
}

void AccountManagerImpl::AddLockedBalance(const Order& order) {
  AdjustLockedBalance(order, std::plus<>{});
}

void AccountManagerImpl::SubtractLockedBalance(const Order& order) {
  AdjustLockedBalance(order, std::minus<>{});
}

void AccountManagerImpl::AdjustLockedBalance(const Order& order, std::function<double(double, double)> operation) {
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

void AccountManagerImpl::AdjustBalanceClosedOrder(const Order& order) {
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