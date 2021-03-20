#pragma once

#include "symbol.h"

#include <iostream>
#include <string>

enum Side : int {
  SELL = -1,
  BUY = 1
};

class Order {
public:

  class Builder {
  public:
    Builder() {

    }

    Builder& Id(const std::string& id) {
      m_id = id;
    }

    Order Build() {
      return Order(std::move(m_id));
    }
  private:
    std::string m_id;
  };

  static Builder Builder() {
    return Builder();
  }

  Order() {

  }

  Order(const std::string& id) : m_id(id) {

  }

  Order(std::string&& id) : m_id(id) {

  }

  inline const std::string& GetId() const { return m_id; }


private:
  std::string m_id;
  std::string m_client_id;
  SymbolId m_symbol_id;
  Side m_side;
  // TODO: more

  friend std::ostream &operator<<(std::ostream &os, const Order &res);
};

std::ostream &operator<<(std::ostream &os, const Order &res) {
  os << res.m_id;
  return os;
}