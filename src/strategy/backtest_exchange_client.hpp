#include "exchange/exchange_client.h"
#include "exchange/exchange_listener.h"

#include <fstream>
#include <map>
#include <sstream>
#include <unordered_map>

struct BacktestSettings {
  std::string m_exchange;
  std::vector<SymbolPairId> symbol_pairs;
  double m_fee;
  double m_slippage;
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
    double price, cost;
    if (side == Side::SELL) {
      price = m_ticker.m_bid;
      if (m_ticker.m_bid_vol && m_ticker.m_bid_vol.value() < qty) {
        //qty = m_ticker.m_bid_vol.value();
        throw std::runtime_error("Not enough qty for current bid price."
            "Implement strategy, so that there is no order with quantity higher than there is available order book for best ask/bid.");
      }
      if (m_balances.at(base_asset_id) < qty) {
        BOOST_LOG_TRIVIAL(warning) << "Not enough qty of " << base_asset_id << " available.";
        return Result<Order>("", "");
      }
      cost = qty*(m_ticker.m_bid - m_settings.m_slippage)*(1.0 - m_settings.m_fee);
      m_balances[quote_asset_id] = m_balances.at(quote_asset_id) + cost;
      m_balances[base_asset_id] = m_balances.at(base_asset_id) - qty;
    } else { // Buying
      price = m_ticker.m_ask;
      if (m_ticker.m_ask_vol && m_ticker.m_ask_vol.value() < qty) {
        //qty = m_ticker.m_ask_vol.value();
        throw std::runtime_error("Not enough qty for current ask price."
            "Implement strategy, so that there is no order with quantity higher than there is available order book for best ask/bid.");
      }
      cost = qty*(m_ticker.m_ask + m_settings.m_slippage)*(1.0 + m_settings.m_fee);
      if (m_balances[quote_asset_id] < cost) {
        BOOST_LOG_TRIVIAL(warning) << "Not enough " << quote_asset_id;
        return Result<Order>("", "");
      }
      m_balances[quote_asset_id] = m_balances[quote_asset_id] - cost;
      m_balances[base_asset_id] = m_balances[base_asset_id] + qty;
    }
    PrintBalances();
    return Result<Order>("", Order("ABC", "ABC", symbol, side, OrderType::MARKET, qty));
  }

  virtual Result<Order> LimitOrder(SymbolPairId symbol, Side side, double qty, double price) override {
    SymbolPair sp{symbol};
    SymbolId base_asset_id = sp.GetBaseAsset();
    SymbolId quote_asset_id = sp.GetQuoteAsset();
    double current_price;
    if (side == Side::SELL) {
      current_price = m_ticker.m_bid;
      if (price <= current_price) {
        return MarketOrder(symbol, side, qty);
      }
      return AddLimitOrder(symbol, side, qty, price);
    }
    // BUY
    current_price = m_ticker.m_ask;
    if (price >= current_price) {
      return MarketOrder(symbol, side, qty);
    }
    return AddLimitOrder(symbol, side, qty, price);
  }

  virtual Result<AccountBalance> GetAccountBalance() override {
    // Assume there are always some coins available
    std::unordered_map<SymbolId, double> asset_balances = {
      {SymbolId::BTC, 0.2},
      {SymbolId::ADA, 5000.0},
      {SymbolId::ETH, 1.0},
      {SymbolId::USDT, 10000.0}
    };
    AccountBalance account_balance{std::move(asset_balances)};
    return Result<AccountBalance>("", std::move(account_balance));
  }

  virtual Result<std::vector<Order>> GetOpenOrders() override {
    return Result<std::vector<Order>>("", std::vector<Order>());
  }

  virtual void CancelAllOrders() override {

  }

  virtual void OnBookTicker(const Ticker& ticker) override {
    if (m_settings.m_exchange == ticker.m_exchange) {
      m_ticker = ticker;
      // Check if any limit order got filled
      SymbolId base_asset_id = m_ticker.m_symbol.GetBaseAsset();
      SymbolId quote_asset_id = m_ticker.m_symbol.GetQuoteAsset();
      size_t i = 0;
      while (i < m_limit_orders.size()) {
        const Order& order = m_limit_orders[i];
        if (order.GetSide() == Side::SELL) {
          if (m_ticker.m_bid >= order.GetPrice()) {
            // Fill
            // TODO: implement partial fill ?
            double cost = m_ticker.m_bid * order.GetQuantity() * (1.0 - m_settings.m_fee);
            m_balances[quote_asset_id] = m_balances.at(quote_asset_id) + cost;
            m_balances[base_asset_id] = m_balances.at(base_asset_id) - order.GetQuantity();
            m_limit_orders.erase(m_limit_orders.begin() + i);
            PrintBalances();
            continue;
          }
        } else { // BUY
          if (m_ticker.m_ask <= order.GetPrice()) {
            // Fill
            // TODO: implement partial fill ?
            double cost = m_ticker.m_ask * order.GetQuantity() * (1.0 + m_settings.m_fee);
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

  void PrintBalances() const {
    const size_t sz = m_balances.size();
    for (auto it = m_balances.begin(); it != m_balances.end(); ++it) {
      const SymbolId& spid = it->first;
      if (std::next(it) != m_balances.end()) {
        // TDOO: FIXME: set correct precision!!
        m_results_file << std::to_string(m_balances.at(spid)) << ",";
      } else {
        m_results_file << std::to_string(m_balances.at(spid)) << "\n";
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
  std::map<SymbolId, double> m_balances;
  std::vector<Order> m_limit_orders;
  Ticker m_ticker;
  std::ofstream m_results_file;
};