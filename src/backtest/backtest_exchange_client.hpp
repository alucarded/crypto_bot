#pragma once

#include "exchange/account_balance_listener.h"
#include "exchange/account_manager.h"
#include "exchange/exchange_listener.h"

#include <chrono>
#include <iostream>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>

using namespace std::chrono;

struct BacktestSettings {
  std::string exchange;
  double fee;
  double slippage;
  // Time it takes for request/response to get to/from exchange server
  double network_latency_us;
  // Time it takes for order to be executed after reaching exchange server
  double execution_delay_us;
  std::unordered_map<SymbolId, double> initial_balances;
};

struct OrderRequest {

  Order order;
  double creation_timestamp_us;
};

// TODO: support order book updates and trade tickers
class BacktestExchangeClient : public AccountManager, public ExchangeListener {
public:
  BacktestExchangeClient(const BacktestSettings& settings, AccountBalanceListener& balance_listener) : m_settings(settings),
  m_account_balance(settings.initial_balances, settings.exchange), m_update_timestamp(0), m_balance_listener(balance_listener), m_last_order_id(0) {
  }

  virtual ~BacktestExchangeClient() {
  }

  // ExchangeClient

  virtual std::string GetExchange() override {
    return m_settings.exchange;
  }

  virtual Result<Order> MarketOrder(SymbolPairId symbol, Side side, double qty) override {
    Order order(std::to_string(++m_last_order_id), std::to_string(m_last_order_id), symbol, side, OrderType::MARKET, qty);
    OrderRequest order_req;
    order_req.order = std::move(order);
    // Here we do not take into account strategy execution delay for simplicity.
    // Use network_latency_us and execution_delay_us to adjust delay.
    //
    // This works with assumption that orders are placed right after receiving a ticker.
    // OnBookTicker with the most recent ticker has to be called before placing an order.
    // There m_update_timestamp is updated with ticker arrival timestamp.
    order_req.creation_timestamp_us = m_update_timestamp_us;
    m_pending_market_orders.push_back(order_req);
    return Result<Order>("", order_req.order);
  }

  virtual Result<Order> LimitOrder(SymbolPairId symbol, Side side, double qty, double price) override {
    return AddLimitOrder(symbol, side, qty, price);
  }

  virtual Result<AccountBalance> GetAccountBalance() override {
    return Result<AccountBalance>("", m_account_balance);
  }

  virtual Result<std::vector<Order>> GetOpenOrders() override {
    return Result<std::vector<Order>>("", m_limit_orders);
  }

  virtual void CancelAllOrders() override {

  }

  // AccountManager

  virtual bool HasOpenOrders() override {
    return !m_limit_orders.empty();
  };

  virtual bool HasOpenOrders(SymbolPairId pair) override {
    for (const auto& order : m_limit_orders) {
      if (pair == order.GetSymbolId()) {
        return true;
      }
    }
    return false;
  };

  virtual double GetFreeBalance(SymbolId symbol_id) override {
    return m_account_balance.GetFreeBalance(symbol_id);
  };

  virtual double GetTotalBalance(SymbolId symbol_id) override {
    return m_account_balance.GetTotalBalance(symbol_id);
  };

  virtual bool IsAccountSynced() const override {
    // TODO: return false if there is a pending market order
    return true;
  };

  // ExchangeListener

  virtual void OnConnectionOpen(const std::string& name) override {
    // no-op
  }

  virtual void OnConnectionClose(const std::string& name) override {
    // no-op
  }

  // TODO: using trade tickers as well (and perhaps with partial fill) would make the backtester better
  virtual void OnBookTicker(const Ticker& ticker) override {
    //BOOST_LOG_TRIVIAL(trace) << "BacktestExchangeClient::OnBookTicker, ticker: " << ticker;
    m_update_timestamp_us = ticker.m_arrived_ts;
    if (m_settings.exchange == ticker.m_exchange) {
      HandlePendingMarketOrders(ticker);
      HandlePendingLimitOrders(ticker);
      HandleLimitOrders(ticker);
      // auto it = m_tickers.find(ticker.m_symbol);
      // if (it != m_tickers.end()) {
      //   m_previous_tickers.insert_or_assign(it->first, it->second);
      // }
      // Assign new best ticker after handling orders
      m_tickers.insert_or_assign(ticker.m_symbol, ticker);
    }
  }

private:

  // TODO: move the private methods to a separate class ? (Backtest)OrderExecutionEngine?

  Result<Order> AddLimitOrder(SymbolPairId symbol, Side side, double qty, double price) {
      Order::Builder order_builder = Order::CreateBuilder();
      Order order = order_builder.Id(std::to_string(++m_last_order_id))
          .ClientId(std::to_string(m_last_order_id))
          .Symbol(symbol)
          .Side_(side)
          .OrderType_(OrderType::LIMIT)
          .Quantity(qty)
          .Price(price)
          .OrderStatus_(OrderStatus::NEW)
          .Build();
      OrderRequest order_req;
      order_req.order = std::move(order);
      order_req.creation_timestamp_us = m_update_timestamp_us;
      m_pending_limit_orders.push_back(order_req);
      return Result<Order>("", order);
  }

  void HandlePendingMarketOrders(const Ticker& ticker) {
    for (size_t i = 0; m_pending_market_orders.size(); ++i) {
      const auto& order_request = m_pending_market_orders[i];
      if (order_request.order.GetSymbolId() != ticker.m_symbol) {
        continue;
      }
      // Take network latency twice: once for incoming data delay and once for sending order delay
      if (order_request.creation_timestamp_us + 2 * m_settings.network_latency_us + m_settings.execution_delay_us < ticker.m_arrived_ts) {
        // Get ticker before that
        auto it = m_tickers.find(ticker.m_symbol);
        if (it != m_tickers.end()) {
          ExecuteMarketOrder(order_request.order, it->second);
        } else {
          BOOST_LOG_TRIVIAL(error) << "No previous ticker";
          // TODO: is it even possible to end up here?
          //ExecuteMarketOrder(order_request.order, ticker);
        }
      }
    }
  }

  void ExecuteMarketOrder(const Order& order, const Ticker& ticker) {
    const SymbolPair& sp = ticker.m_symbol;
    SymbolId base_asset_id = sp.GetBaseAsset();
    SymbolId quote_asset_id = sp.GetQuoteAsset();
    Side side = order.GetSide();
    double qty = order.GetQuantity();
    double price, cost;
    if (side == Side::SELL) {
      price = ticker.m_bid;
      BOOST_LOG_TRIVIAL(info) << "Current ticker " << ticker << ", market selling " << qty;
      // TODO: FIXME: implement partial fill here
      if (ticker.m_bid_vol && ticker.m_bid_vol.value() < qty) {
        BOOST_LOG_TRIVIAL(info) << "Order quantity above best ticker volume";
      }
      cost = qty*(price - m_settings.slippage)*(1.0 - m_settings.fee);
      m_account_balance.AddBalance(quote_asset_id, cost);
      m_account_balance.AddBalance(base_asset_id, -qty);
      BOOST_LOG_TRIVIAL(info) << "Sold " << qty << " for " << cost;
    } else { // Buying
      price = ticker.m_ask;
      BOOST_LOG_TRIVIAL(info) << "Current ticker " << ticker << ", market buying " << qty;
      if (ticker.m_ask_vol && ticker.m_ask_vol.value() < qty) {
        BOOST_LOG_TRIVIAL(info) << "Order quantity above best ticker volume";
      }
      cost = qty*(price + m_settings.slippage)*(1.0 + m_settings.fee);
      m_account_balance.AddBalance(quote_asset_id, -cost);
      m_account_balance.AddBalance(base_asset_id, qty);
      BOOST_LOG_TRIVIAL(info) << "Bought " << qty << " for " << cost;
    }
    m_balance_listener.OnAccountBalanceUpdate(m_account_balance);
  }

  void HandlePendingLimitOrders(const Ticker& ticker) {
    for (size_t i = 0; m_pending_limit_orders.size(); ++i) {
      const auto& order_request = m_pending_limit_orders[i];
      const auto& order = order_request.order;
      if (order.GetSymbolId() != ticker.m_symbol) {
        continue;
      }
      // Take network latency twice: once for incoming data delay and once for sending order delay
      if (order_request.creation_timestamp_us + 2 * m_settings.network_latency_us + m_settings.execution_delay_us < ticker.m_arrived_ts) {
        double price = order.GetPrice();
        Side side = order.GetSide();
        if (Side::SELL == side) {
          if (price <= ticker.m_bid) {
            ExecuteMarketOrder(order, ticker);
          } else {
            m_limit_orders.push_back(order);
          }
        } else { // BUY
          if (price >= ticker.m_ask) {
            ExecuteMarketOrder(order, ticker);
          } else {
            m_limit_orders.push_back(order);
          }
        }
      }
    }
  }

  void HandleLimitOrders(const Ticker& ticker) {
    // Check if any limit order got filled
    SymbolId base_asset_id = ticker.m_symbol.GetBaseAsset();
    SymbolId quote_asset_id = ticker.m_symbol.GetQuoteAsset();
    for (size_t i = 0; i < m_limit_orders.size(); ++i) {
      const auto& order = m_limit_orders[i];
      if (order.GetSymbolId() != ticker.m_symbol) {
        continue;
      }
      double order_price = order.GetPrice();
      if (order.GetSide() == Side::SELL) {
        if (ticker.m_bid >= order_price) {
          // Fill
          // TODO: implement partial fill ?
          BOOST_LOG_TRIVIAL(info) << "Filled sell limit order: " << order;
          double cost = ticker.m_bid * order.GetQuantity() * (1.0 - m_settings.fee);
          m_account_balance.AddBalance(quote_asset_id, cost);
          m_account_balance.AddBalance(base_asset_id, -order.GetQuantity());
          m_limit_orders.erase(m_limit_orders.begin() + i);
          m_balance_listener.OnAccountBalanceUpdate(m_account_balance);
          continue;
        }
      } else { // BUY
        if (ticker.m_ask <= order_price) {
          // Fill
          // TODO: implement partial fill ?
          BOOST_LOG_TRIVIAL(info) << "Filled buy limit order: " << order;
          double cost = ticker.m_ask * order.GetQuantity() * (1.0 + m_settings.fee);
          m_account_balance.AddBalance(quote_asset_id, -cost);
          m_account_balance.AddBalance(base_asset_id, order.GetQuantity());
          m_limit_orders.erase(m_limit_orders.begin() + i);
          m_balance_listener.OnAccountBalanceUpdate(m_account_balance);
          continue;
        }
      }
    }
  }
private:
  BacktestSettings m_settings;
  AccountBalance m_account_balance;
  std::vector<OrderRequest> m_pending_market_orders;
  std::vector<OrderRequest> m_pending_limit_orders;
  std::vector<Order> m_limit_orders;
  uint64_t m_update_timestamp_us;
  std::unordered_map<SymbolPairId, Ticker> m_tickers;
  // std::unordered_map<SymbolPairId, Ticker> m_previous_tickers;
  AccountBalanceListener& m_balance_listener;
  uint64_t m_last_order_id;
};