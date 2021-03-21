#pragma once

#include "symbol.h"

#include <iostream>
#include <string>

enum class Side : int {
  SELL = -1,
  BUY = 1
};

enum class OrderType : int {
  MARKET,
  LIMIT,
  UNKNOWN
};

enum class ExecutionType : int {
  NEW,
  CANCELED,
  REPLACED,
  REJECTED,
  TRADE,
  EXPIRED,
  UNKNOWN
};

enum class OrderStatus : int {
  NEW,
  PARTIALLY_FILLED,
  FILLED,
  CANCELED,
  PENDING_CANCEL,
  REJECTED,
  EXPIRED,
  UNKNOWN
};

class Order {
public:

  static OrderType GetType(const std::string& type_str) {
    static std::unordered_map<std::string, OrderType> s_order_type_map = {
      {"MARKET", OrderType::MARKET},
      {"LIMIT", OrderType::LIMIT}
    };
    auto it = s_order_type_map.find(type_str);
    if (it != s_order_type_map.end()) {
      return it->second;
    }
    BOOST_LOG_TRIVIAL(warning) << "Unknown order type " << type_str;
    return OrderType::UNKNOWN;
  }

  static ExecutionType GetExecutionType(const std::string& exec_type_str) {
    static std::unordered_map<std::string, ExecutionType> s_execution_type_map = {
      {"NEW", ExecutionType::NEW},
      {"CANCELED", ExecutionType::CANCELED},
      {"REPLACED", ExecutionType::REPLACED},
      {"REJECTED", ExecutionType::REJECTED},
      {"TRADE", ExecutionType::TRADE},
      {"EXPIRED", ExecutionType::EXPIRED},
    };
    auto it = s_execution_type_map.find(exec_type_str);
    if (it != s_execution_type_map.end()) {
      return it->second;
    }
    BOOST_LOG_TRIVIAL(warning) << "Unknown execution type " << exec_type_str;
    return ExecutionType::UNKNOWN;
  }

  static OrderStatus GetStatus(const std::string& status_str) {
    static std::unordered_map<std::string, OrderStatus> s_status_map = {
      {"NEW", OrderStatus::NEW},
      {"PARTIALLY_FILLED", OrderStatus::PARTIALLY_FILLED},
      {"FILLED", OrderStatus::FILLED},
      {"CANCELED", OrderStatus::CANCELED},
      {"PENDING_CANCEL", OrderStatus::PENDING_CANCEL},
      {"REJECTED", OrderStatus::REJECTED},
      {"EXPIRED", OrderStatus::EXPIRED}
    };
    auto it = s_status_map.find(status_str);
    if (it != s_status_map.end()) {
      return it->second;
    }
    BOOST_LOG_TRIVIAL(warning) << "Unknown order status " << status_str;
    return OrderStatus::UNKNOWN;
  }

  class Builder {
  public:
    Builder() {

    }

    Builder& Id(const std::string& id) {
      m_id = id;
      return *this;
    }

    Builder& ClientId(const std::string& client_id) {
      m_client_id = client_id;
      return *this;
    }

    Builder& Symbol(SymbolPairId symbol_id) {
      m_symbol_id = symbol_id;
      return *this;
    }

    Builder& Side_(Side side) {
      m_side = side;
      return *this;
    }

    Builder& OrderType_(OrderType order_type) {
      m_order_type = order_type;
      return *this;
    }

    Builder& Quantity(double quantity) {
      m_quantity = quantity;
      return *this;
    }

    Builder& Price(double price) {
      m_price = price;
      return *this;
    }

    Builder& ExecutionType_(ExecutionType exec_type) {
      m_execution_type = exec_type;
      return *this;
    }

    Builder& OrderStatus_(OrderStatus order_status) {
      m_order_status = order_status;
      return *this;
    }

    Order Build() {
      return Order(std::move(m_id), std::move(m_client_id), m_symbol_id, m_side, m_order_type, m_quantity, m_price, m_execution_type, m_order_status);
    }
  private:
    std::string m_id;
    std::string m_client_id;
    SymbolPairId m_symbol_id;
    Side m_side;
    OrderType m_order_type;
    double m_quantity;
    double m_price;
    ExecutionType m_execution_type;
    OrderStatus m_order_status;
  };

  static Builder Builder() {
    return Builder();
  }

  Order() {

  }

  Order(const std::string& id) : m_id(id) {

  }

  Order(std::string&& id, std::string&& client_id, SymbolPairId symbol_id, Side side,
      OrderType order_type, double quantity, double price, ExecutionType execution_type, OrderStatus order_status)
    : m_id(id), m_client_id(client_id), m_symbol_id(symbol_id), m_side(side), m_order_type(order_type),
      m_quantity(quantity), m_price(price), m_execution_type(execution_type), m_order_status(order_status) {

  }

  inline const std::string& GetId() const { return m_id; }


private:
  std::string m_id;
  std::string m_client_id;
  SymbolPairId m_symbol_id;
  Side m_side;
  OrderType m_order_type;
  double m_quantity;
  double m_price;
  ExecutionType m_execution_type;
  OrderStatus m_order_status;
  // TODO: more

  friend std::ostream &operator<<(std::ostream &os, const Order &res);
};

std::ostream &operator<<(std::ostream &os, const Order &res) {
  os << res.m_id;
  return os;
}