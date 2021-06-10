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

class BacktestExchangeClient : public AccountManager, public ExchangeListener {
public:
  BacktestExchangeClient(const BacktestSettings& settings, AccountBalanceListener& balance_listener) : m_settings(settings),
  m_account_balance(settings.initial_balances, settings.exchange), m_balance_listener(balance_listener), m_last_order_id(0) {
  }

  virtual ~BacktestExchangeClient() {
  }

  // ExchangeClient

  virtual std::string GetExchange() override {
    return m_settings.exchange;
  }

  virtual Result<Order> MarketOrder(SymbolPairId symbol, Side side, double qty) override {
    m_pending_market_orders.emplace_back(std::to_string(++m_last_order_id), std::to_string(m_last_order_id), symbol, side, OrderType::MARKET, qty);
    return Result<Order>("", Order("ABC", "ABC", symbol, side, OrderType::MARKET, qty));
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

  virtual void OnBookTicker(const Ticker& ticker) override {
    //BOOST_LOG_TRIVIAL(trace) << "BacktestExchangeClient::OnBookTicker, ticker: " << ticker;
    if (m_settings.exchange == ticker.m_exchange) {
      HandlePendingMarketOrders(ticker);
      HandleLimitOrders(ticker);
      // Assign new best ticker after handling orders
      m_tickers.insert_or_assign(ticker.m_symbol, ticker);
    }
  }

private:

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
      m_limit_orders.push_back(order);
      return Result<Order>("", order);
  }

  void HandlePendingMarketOrders(const Ticker& ticker) {
    for (size_t i = 0; m_pending_market_orders.size(); ++i) {
      const auto& order_request = m_pending_market_orders[i];
      if (order_request.order.GetSymbolId() != ticker.m_symbol) {
        continue;
      }
      if (order_request.creation_time_us + network_latency_us + execution_delay_us < ticker.m_arrived_ts) {
        // Get ticker before that
        auto it = m_tickers.find(ticker.m_symbol);
        if (it != m_tickers.end()) {}
          ExecuteMarketOrder(order_request.order, *it);
        } else {
          BOOST_LOG_TRIVIAL(info) << "Executing market order with incoming ticker";
          ExecuteMarketOrder(order_request.order, ticker);
        }
      }
    }
  }

  void ExecuteMarketOrder(const Order& order, const Ticker& ticker) {
    const std::string& symbol = ticker.m_symbol;
    SymbolPair sp{symbol};
    SymbolId base_asset_id = sp.GetBaseAsset();
    SymbolId quote_asset_id = sp.GetQuoteAsset();
    double qty = order.GetQuantity();
    double price, cost;
    if (side == Side::SELL) {
      price = ticker.m_bid;
      BOOST_LOG_TRIVIAL(info) << "Current ticker " << ticker << ", selling " << qty;
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
      BOOST_LOG_TRIVIAL(info) << "Current ticker " << ticker << ", buying " << qty;
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
  std::vector<OrderRequest> m_limit_orders;
  std::unordered_map<SymbolPairId, Ticker> m_tickers;
  std::unordered_map<SymbolPairId, Ticker> m_previous_tickers;
  AccountBalanceListener& m_balance_listener;
  uint64_t m_last_order_id;
};