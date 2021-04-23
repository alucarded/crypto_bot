#pragma once

#include "symbol.h"

#include <iostream>
#include <string>
#include <unordered_map>

struct AccountBalance {
  AccountBalance() {
    // TODO: remove
  }

  AccountBalance(std::unordered_map<SymbolId, double>&& asset_balance_map)
      : m_asset_balance_map(std::move(asset_balance_map)) {
  }

  AccountBalance(AccountBalance&& b) : AccountBalance(std::move(b.m_asset_balance_map)) {
    
  }

  AccountBalance& operator=(AccountBalance&& account_balance) {
    m_asset_balance_map = std::move(account_balance.m_asset_balance_map);
    m_locked_balance_map = std::move(account_balance.m_locked_balance_map);
    return *this;
  }

  double GetTotalBalance(SymbolId asset_name) const {
    if (m_asset_balance_map.count(asset_name) < 1) {
      return 0;
    }
    return m_asset_balance_map.at(asset_name);
  }

  double GetLockedBalance(SymbolId asset_name) const {
    if (m_locked_balance_map.count(asset_name) < 1) {
      return 0;
    }
    return m_locked_balance_map.at(asset_name);
  }

  double GetFreeBalance(SymbolId asset_name) const {
    return GetTotalBalance(asset_name) - GetLockedBalance(asset_name);
  }

  void SetTotalBalance(SymbolId asset_name, double val) {
    m_asset_balance_map.insert_or_assign(asset_name, val);
  }

  void SetLockedBalance(SymbolId asset_name, double val) {
    m_locked_balance_map.insert_or_assign(asset_name, val);
  }

  const std::unordered_map<SymbolId, double>& GetBalanceMap() const {
    return m_asset_balance_map;
  }

  void UpdateTotalBalance(const AccountBalance& other) {
    for (const auto& p : other.GetBalanceMap()) {
      SetTotalBalance(p.first, p.second);
    }
  }

  // This should always contain full balance per asset (open orders included)
  std::unordered_map<SymbolId, double> m_asset_balance_map;
  // Only locked balance (by orders)
  // TODO:
  std::unordered_map<SymbolId, double> m_locked_balance_map;
  uint64_t m_last_update;

  friend std::ostream &operator<<(std::ostream &os, const AccountBalance &res);
};

std::ostream &operator<<(std::ostream &os, const AccountBalance &res) {
  os << "{ \"asset_balance_map\": { ";
  for (auto it = res.m_asset_balance_map.begin(); it != res.m_asset_balance_map.end(); ++it) {
    os << "\"" << it->first << "\": \"" << std::to_string(it->second) << "\"" << (std::next(it) != res.m_asset_balance_map.end() ? ", " : " ");
  }
  os << "}";
  os << ", ";
  os << "{ \"locked_balance_map\": { ";
  for (auto it = res.m_locked_balance_map.begin(); it != res.m_locked_balance_map.end(); ++it) {
    os << "\"" << it->first << "\": \"" << std::to_string(it->second) << "\"" << (std::next(it) != res.m_locked_balance_map.end() ? ", " : " ");
  }
  os << "}";
  return os;
}
