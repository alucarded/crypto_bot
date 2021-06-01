#pragma once

#include "exchange/account_balance_listener.h"
#include "exchange/exchange_client.h"
#include "exchange/exchange_listener.h"

#include <iostream>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>

struct BacktestSettings {
  std::string exchange;
  double fee;
  double slippage;
};

class BacktestExchangeClient : public ExchangeClient, public ExchangeListener {
public:
  BacktestExchangeClient(const BacktestSettings& settings, AccountBalanceListener& balance_listener) : m_settings(settings),
  m_account_balance(settings.exchange), m_balance_listener(balance_listener) {
    m_account_balance.SetTotalBalance(SymbolId::ADA, 10000.0);
    m_account_balance.SetTotalBalance(SymbolId::BTC, 1.0);
    m_account_balance.SetTotalBalance(SymbolId::ETH, 10.0);
    m_account_balance.SetTotalBalance(SymbolId::USDT, 24000);
  }

  virtual ~BacktestExchangeClient() {
    m_results_file.close();
  }

  virtual std::string GetExchange() override {
    return m_settings.exchange;
  }

  virtual Result<Order> MarketOrder(SymbolPairId symbol, Side side, double qty) override {
    //std::cout << "Market order " << ((side == 1) ? "BUY" : "SELL") << std::endl;
    // TODO: for now BTCUSD (BTCUSDT) assumed, add enum for symbols, data types (tickers, order book etc ?) and exchanges
    SymbolPair sp{symbol};
    SymbolId base_asset_id = sp.GetBaseAsset();
    SymbolId quote_asset_id = sp.GetQuoteAsset();
    assert(m_tickers.find(symbol) != m_tickers.end());
    const Ticker& ticker = m_tickers.at(symbol);
    double price, cost;
    if (side == Side::SELL) {
      price = ticker.m_bid;
      BOOST_LOG_TRIVIAL(info) << "Current ticker " << ticker << ", selling " << qty;
      if (ticker.m_bid_vol && ticker.m_bid_vol.value() < qty) {
        //qty = ticker.m_bid_vol.value();
        throw std::runtime_error("Not enough qty for current bid price."
            "Implement strategy, so that there is no order with quantity higher than there is available order book for best ask/bid.");
      }
      // if (m_balances.at(base_asset_id) < qty) {
      //   BOOST_LOG_TRIVIAL(warning) << "Not enough qty of " << base_asset_id << " available.";
      //   return Result<Order>("", "");
      // }
      cost = qty*(price - m_settings.slippage)*(1.0 - m_settings.fee);
      m_account_balance.AddBalance(quote_asset_id, cost);
      m_account_balance.AddBalance(base_asset_id, -qty);
      BOOST_LOG_TRIVIAL(info) << "Sold " << qty << " for " << cost;

    } else { // Buying
      price = ticker.m_ask;
      BOOST_LOG_TRIVIAL(info) << "Current ticker " << ticker << ", buying " << qty;
      if (ticker.m_ask_vol && ticker.m_ask_vol.value() < qty) {
        //qty = ticker.m_ask_vol.value();
        throw std::runtime_error("Not enough qty for current ask price."
            "Implement strategy, so that there is no order with quantity higher than there is available order book for best ask/bid.");
      }
      cost = qty*(price + m_settings.slippage)*(1.0 + m_settings.fee);
      // if (m_balances[quote_asset_id] < cost) {
      //   BOOST_LOG_TRIVIAL(warning) << "Not enough " << quote_asset_id;
      //   return Result<Order>("", "");
      // }
      m_account_balance.AddBalance(quote_asset_id, -cost);
      m_account_balance.AddBalance(base_asset_id, qty);
      BOOST_LOG_TRIVIAL(info) << "Bought " << qty << " for " << cost;
    }
    m_balance_listener.OnBalanceUpdate(m_account_balance);
    return Result<Order>("", Order("ABC", "ABC", symbol, side, OrderType::MARKET, qty));
  }

  virtual Result<Order> LimitOrder(SymbolPairId symbol, Side side, double qty, double price) override {
    SymbolPair sp{symbol};
    SymbolId base_asset_id = sp.GetBaseAsset();
    SymbolId quote_asset_id = sp.GetQuoteAsset();
    assert(m_tickers.find(symbol) != m_tickers.end());
    const Ticker& ticker = m_tickers.at(symbol);
    double current_price;
    if (side == Side::SELL) {
      current_price = ticker.m_bid;
      if (price <= current_price) {
        return MarketOrder(symbol, side, qty);
      }
      return AddLimitOrder(symbol, side, qty, price);
    }
    // BUY
    current_price = ticker.m_ask;
    if (price >= current_price) {
      return MarketOrder(symbol, side, qty);
    }
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

  virtual void OnBookTicker(const Ticker& ticker) override {
    //BOOST_LOG_TRIVIAL(trace) << "BacktestExchangeClient::OnBookTicker, ticker: " << ticker;
    if (m_settings.exchange == ticker.m_exchange) {
      m_tickers.insert_or_assign(ticker.m_symbol, ticker);

      // Check if any limit order got filled
      SymbolId base_asset_id = ticker.m_symbol.GetBaseAsset();
      SymbolId quote_asset_id = ticker.m_symbol.GetQuoteAsset();
      size_t i = 0;
      while (i < m_limit_orders.size()) {
        const Order& order = m_limit_orders[i];
        double order_price = order.GetPrice();
        if (order.GetSide() == Side::SELL) {
          if (ticker.m_bid >= order_price) {
            // Fill
            // TODO: implement partial fill ?
            BOOST_LOG_TRIVIAL(info) << "Filled sell limit order: " << order;
            double cost = order_price * order.GetQuantity() * (1.0 - m_settings.fee);
            m_account_balance.AddBalance(quote_asset_id, cost);
            m_account_balance.AddBalance(base_asset_id, -order.GetQuantity());
            m_limit_orders.erase(m_limit_orders.begin() + i);
            m_balance_listener.OnBalanceUpdate(m_account_balance);
            continue;
          }
        } else { // BUY
          if (ticker.m_ask <= order_price) {
            // Fill
            // TODO: implement partial fill ?
            BOOST_LOG_TRIVIAL(info) << "Filled buy limit order: " << order;
            double cost = order_price * order.GetQuantity() * (1.0 + m_settings.fee);
            m_account_balance.AddBalance(quote_asset_id, -cost);
            m_account_balance.AddBalance(base_asset_id, order.GetQuantity());
            m_limit_orders.erase(m_limit_orders.begin() + i);
            m_balance_listener.OnBalanceUpdate(m_account_balance);
            continue;
          }
        }
        ++i;
      }
    }
  }

private:

  Result<Order> AddLimitOrder(SymbolPairId symbol, Side side, double qty, double price) {
      Order::Builder order_builder = Order::CreateBuilder();
      Order order = order_builder.Id("ABC")
          .ClientId("ABC")
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

private:
  BacktestSettings m_settings;
  AccountBalance m_account_balance;
  std::vector<Order> m_limit_orders;
  std::unordered_map<SymbolPairId, Ticker> m_tickers;
  std::ofstream m_results_file;
  AccountBalanceListener& m_balance_listener;
};