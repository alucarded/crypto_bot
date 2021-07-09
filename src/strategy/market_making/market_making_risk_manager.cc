#include "strategy/market_making/market_making_risk_manager.h"

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <algorithm>

MarketMakingRiskManager::MarketMakingRiskManager(const MarketMakingRiskMangerOptions& opts, ExchangeClient* exchange_client)
    : m_options(opts), m_exchange_client(exchange_client), m_trading_balance(0) {

}

MarketMakingRiskManager::~MarketMakingRiskManager() {

}

void MarketMakingRiskManager::OnConnectionOpen(const std::string&) {

}

void MarketMakingRiskManager::OnConnectionClose(const std::string&) {

}

void MarketMakingRiskManager::OnAccountBalanceUpdate(const AccountBalance& account_balance) {
  std::scoped_lock<std::mutex> order_lock{m_order_mutex};
  m_account_balance = account_balance;
}

void MarketMakingRiskManager::OnOrderUpdate(const Order& order) {
  std::scoped_lock<std::mutex> order_lock{m_order_mutex};
  auto it = std::find(m_orders.begin(), m_orders.end(), order);
  if (it != m_orders.end()) {
    if (OrderStatus::FILLED == order.GetStatus()) {
      BOOST_LOG_TRIVIAL(info) << "Order filled: " << order;
      m_orders.erase(it);
      if (order.GetSide() == Side::SELL) {
        m_trading_balance -= order.GetExecutedQuantity();
      } else { // Side::BUY
        m_trading_balance += order.GetExecutedQuantity();
      }
      BOOST_LOG_TRIVIAL(info) << "Bot trading balance: " << m_trading_balance;
    }
  }
}

void MarketMakingRiskManager::OnPricePrediction(const MarketMakingPrediction& prediction) {
  std::scoped_lock<std::mutex> order_lock{m_order_mutex};
  // Expire orders
  // for (size_t i = 0; i < m_orders.size(); ++i) {
  //   const auto& order = m_orders[i];
  //   if (prediction.timestamp_us - order.GetCreationTime() > m_options.order_expiration_us) {
  //     BOOST_LOG_TRIVIAL(info) << "Order expired: " << order;
  //     m_exchange_client->CancelOrder(order);
  //     m_orders.erase(m_orders.begin() + i);
  //     --i;
  //   }
  // }
  // Add new orders
  std::vector<Order> orders = CalculateOrders(prediction);
  for (const auto& order : orders) {
    m_orders.push_back(order);
    m_exchange_client->SendOrder(order);
  }
}

std::vector<Order> MarketMakingRiskManager::CalculateOrders(const MarketMakingPrediction& prediction) {
  std::vector<Order> res;
  if (!m_orders.empty()) {
    BOOST_LOG_TRIVIAL(debug) << "Has open orders, so not sending new ones";
    return {};
  }
  double sell_price = prediction.base_price/(1.0d - m_options.exchange_fee - m_options.our_fee);
  sell_price += prediction.signal*(sell_price - prediction.base_price);
  double buy_price = prediction.base_price/(1.0d + m_options.exchange_fee + m_options.our_fee);
  buy_price += prediction.signal*(prediction.base_price - buy_price);
  double sell_quantity = m_options.default_order_qty;
  // if (m_trading_balance > 0) {
  //   sell_quantity += m_trading_balance;
  // }
  double buy_quantity = m_options.default_order_qty;
  // if (m_trading_balance < 0) {
  //   buy_quantity -= m_trading_balance;
  // }
  //boost::uuids::uuid sell_order_id(boost::uuids::random_generator()());
  std::string sell_order_id = boost::uuids::to_string(boost::uuids::random_generator()());
  BOOST_LOG_TRIVIAL(trace) << "Prediction time: " << prediction.timestamp_us;
  Order sell_order = Order::CreateBuilder()
    .Id("")
    .ClientId(sell_order_id)
    .Symbol(prediction.symbol)
    .Side_(Side::SELL)
    .OrderType_(OrderType::LIMIT)
    .Quantity(sell_quantity)
    .Price(sell_price)
    .CreationTime(prediction.timestamp_us)
    .Build();
  res.push_back(sell_order);
  std::string buy_order_id = boost::uuids::to_string(boost::uuids::random_generator()());
  Order buy_order = Order::CreateBuilder()
    .Id("")
    .ClientId(buy_order_id)
    .Symbol(prediction.symbol)
    .Side_(Side::BUY)
    .OrderType_(OrderType::LIMIT)
    .Quantity(buy_quantity)
    .Price(buy_price)
    .CreationTime(prediction.timestamp_us)
    .Build();
  res.push_back(buy_order);
  return res;
}