#include "exchange/exchange_client.h"
#include "exchange/exchange_listener.h"

#include <iostream>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>

struct BacktestSettings {
  std::string m_exchange;
  std::vector<SymbolPairId> symbol_pairs;
  double fee;
  double slippage;
  double m_min_qty;
};

class BacktestExchangeClient : public ExchangeClient, public ExchangeListener {
public:
  BacktestExchangeClient(const BacktestSettings& settings, const std::string& results_file) : m_settings(settings),
      m_results_file(results_file, std::ios::out | std::ios::app | std::ios::binary) {
    m_balances[SymbolId::ADA] = 10000.0;
    m_balances[SymbolId::BTC] = 1.0;
    m_balances[SymbolId::ETH] = 10.0;
    m_balances[SymbolId::USDT] = 24000;
    std::ostringstream oss;
    for (const auto p : m_balances) {
      SymbolId sid = p.first;
      oss << sid << ",";
    }
    std::string header = oss.str();
    // Remove ',' from the end
    header.pop_back();
    m_results_file << header + "\n";
  }

  virtual ~BacktestExchangeClient() {
    m_results_file.close();
  }

  virtual std::string GetExchange() override {
    return "backtest";
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
      m_balances[quote_asset_id] = m_balances.at(quote_asset_id) + cost;
      m_balances[base_asset_id] = m_balances.at(base_asset_id) - qty;
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
      m_balances[quote_asset_id] = m_balances[quote_asset_id] - cost;
      m_balances[base_asset_id] = m_balances[base_asset_id] + qty;
      BOOST_LOG_TRIVIAL(info) << "Bought " << qty << " for " << cost;
    }
    PrintBalances();
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
    // // Assume there are always some coins available
    // std::unordered_map<SymbolId, double> asset_balances = {
    //   {SymbolId::BTC, 0.2},
    //   {SymbolId::ADA, 5000.0},
    //   {SymbolId::ETH, 1.0},
    //   {SymbolId::DOT, 100.0},
    //   {SymbolId::EOS, 1000.0},
    //   {SymbolId::USDT, 10000.0}
    // };
    std::unordered_map<SymbolId, double> asset_balances(m_balances);
    AccountBalance account_balance{std::move(asset_balances)};
    return Result<AccountBalance>("", std::move(account_balance));
  }

  virtual Result<std::vector<Order>> GetOpenOrders() override {
    return Result<std::vector<Order>>("", m_limit_orders);
  }

  virtual void CancelAllOrders() override {

  }

  virtual void OnBookTicker(const Ticker& ticker) override {
    //BOOST_LOG_TRIVIAL(trace) << "BacktestExchangeClient::OnBookTicker, ticker: " << ticker;
    if (m_settings.m_exchange == ticker.m_exchange) {
      m_tickers.insert_or_assign(ticker.m_symbol, ticker);

      // Check if any limit order got filled
      SymbolId base_asset_id = ticker.m_symbol.GetBaseAsset();
      SymbolId quote_asset_id = ticker.m_symbol.GetQuoteAsset();
      size_t i = 0;
      while (i < m_limit_orders.size()) {
        const Order& order = m_limit_orders[i];
        if (order.GetSide() == Side::SELL) {
          if (ticker.m_bid >= order.GetPrice()) {
            // Fill
            // TODO: implement partial fill ?
            double cost = ticker.m_bid * order.GetQuantity() * (1.0 - m_settings.fee);
            m_balances[quote_asset_id] = m_balances.at(quote_asset_id) + cost;
            m_balances[base_asset_id] = m_balances.at(base_asset_id) - order.GetQuantity();
            m_limit_orders.erase(m_limit_orders.begin() + i);
            PrintBalances();
            continue;
          }
        } else { // BUY
          if (ticker.m_ask <= order.GetPrice()) {
            // Fill
            // TODO: implement partial fill ?
            double cost = ticker.m_ask * order.GetQuantity() * (1.0 + m_settings.fee);
            m_balances[quote_asset_id] = m_balances.at(quote_asset_id) - cost;
            m_balances[base_asset_id] = m_balances.at(base_asset_id) + order.GetQuantity();
            m_limit_orders.erase(m_limit_orders.begin() + i);
            PrintBalances();
            continue;
          }
        }
        ++i;
      }
    }
  }

  void PrintBalances() {
    const size_t sz = m_balances.size();
    for (auto it = m_balances.begin(); it != m_balances.end(); ++it) {
      const SymbolId& spid = it->first;
      // TODO: FIXME: set correct precision!!
      std::string b = std::to_string(m_balances.at(spid));
      m_results_file << b;
      if (std::next(it) != m_balances.end()) {
        m_results_file << ",";
      } else {
        m_results_file << "\n";
      }
    }
    m_results_file.flush();
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
  const BacktestSettings m_settings;
  std::unordered_map<SymbolId, double> m_balances;
  std::vector<Order> m_limit_orders;
  std::unordered_map<SymbolPairId, Ticker> m_tickers;
  std::ofstream m_results_file;
};