#pragma once

#include "symbol.h"

#include <iostream>
#include <string>

enum Side : int {
  SELL = -1,
  BUY = 1
};

struct Order {
  Order() {

  }

  Order(const std::string& id) : m_id(id) {

  }

  inline const std::string& GetId() const { return m_id; }


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