#pragma once
#include <iostream>
#include <string>
#include <unordered_map>

enum Side : int {
  ASK = -1,
  BID = 1
};

struct NewOrderResult {
  NewOrderResult(const std::string& response) : m_full_response(response) {}
  std::string m_full_response;
  // TODO:

  friend std::ostream &operator<<(std::ostream &os, const NewOrderResult &res);
};

std::ostream &operator<<(std::ostream &os, const NewOrderResult &res) {
  os << res.m_full_response;
  return os;
}

struct AccountBalance {
  AccountBalance(const std::unordered_map<std::string, std::string>& asset_balance_map) {
    for (auto p : asset_balance_map) {
      m_asset_balance_map.insert(std::make_pair(p.first, std::stod(p.second)));
    }
  }

  AccountBalance(std::unordered_map<std::string, double>&& asset_balance_map)
      : m_asset_balance_map(asset_balance_map) {
  }

  AccountBalance(AccountBalance&& b) : AccountBalance(std::move(b.m_asset_balance_map)) {
    
  }

  AccountBalance& operator=(AccountBalance&& account_balance) {
    m_asset_balance_map = std::move(account_balance.m_asset_balance_map);
    return *this;
  }

  double GetBalance(const std::string& asset_name) const {
    if (m_asset_balance_map.count(asset_name) < 1) {
      return 0;
    }
    return m_asset_balance_map.at(asset_name);
  }

  // TODO: depending on exchange this can contain total asset amount in possession or free amount (not locked by open order etc.)
  std::unordered_map<std::string, double> m_asset_balance_map;

  friend std::ostream &operator<<(std::ostream &os, const AccountBalance &res);
};

std::ostream &operator<<(std::ostream &os, const AccountBalance &res) {
  // TODO:
  for (auto p : res.m_asset_balance_map) {
    os << p.first << "=" << std::to_string(p.second) << ", ";
  }
  return os;
}

class ExchangeClient {
public:
  virtual NewOrderResult MarketOrder(const std::string& symbol, Side side, double qty) = 0;
  virtual NewOrderResult LimitOrder(const std::string& symbol, Side side, double qty, double price) = 0;
  virtual AccountBalance GetAccountBalance() = 0;
  virtual void CancelAllOrders() = 0;
};