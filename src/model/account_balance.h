#pragma once

#include "symbol.h"

#include <iostream>
#include <string>
#include <unordered_map>

struct AccountBalance {
  AccountBalance() {

  }

  AccountBalance(const std::unordered_map<SymbolId, std::string>& asset_balance_map) {
    for (auto p : asset_balance_map) {
      m_asset_balance_map.insert(std::make_pair(p.first, std::stod(p.second)));
    }
  }

  AccountBalance(std::unordered_map<SymbolId, double>&& asset_balance_map)
      : m_asset_balance_map(asset_balance_map) {
  }

  AccountBalance(AccountBalance&& b) : AccountBalance(std::move(b.m_asset_balance_map)) {
    
  }

  AccountBalance& operator=(AccountBalance&& account_balance) {
    m_asset_balance_map = std::move(account_balance.m_asset_balance_map);
    return *this;
  }

  double GetBalance(SymbolId asset_name) const {
    if (m_asset_balance_map.count(asset_name) < 1) {
      return 0;
    }
    return m_asset_balance_map.at(asset_name);
  }

  const std::unordered_map<SymbolId, double>& GetBalanceMap() const {
    return m_asset_balance_map;
  }

  // TODO: depending on exchange this can contain total asset amount in possession or free amount (not locked by open order etc.)
  std::unordered_map<SymbolId, double> m_asset_balance_map;
  uint64_t m_last_update;

  friend std::ostream &operator<<(std::ostream &os, const AccountBalance &res);
};

std::ostream &operator<<(std::ostream &os, const AccountBalance &res) {
  for (auto p : res.m_asset_balance_map) {
    // TODO: use map enum -> string
    os << int(p.first) << "=" << std::to_string(p.second) << ", ";
  }
  return os;
}
