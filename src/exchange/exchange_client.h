#pragma once
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

enum Side : int {
  ASK = -1,
  BID = 1
};

template <typename T>
class Result {
public:
  Result(const std::string& raw_response, const T& obj)
      : m_raw_response(raw_response), m_error_msg(), m_result_object(obj) {

  }

  Result(const std::string& raw_response, T&& obj)
      : m_raw_response(raw_response), m_error_msg(), m_result_object(std::move(obj)) {

  }

  Result(const std::string& raw_response, const std::string& error_msg)
      : m_raw_response(raw_response), m_error_msg(error_msg) {

  }
  // TODO: add move constructors

  // Returns false when error
  explicit operator bool() const { return m_error_msg.empty(); }
  
  inline const std::string& GetErrorMsg() const { return m_error_msg; }

  inline const std::string& GetRawResponse() const { return m_raw_response; }

  inline const T& Get() const { return m_result_object; };
  inline T& Get() { return m_result_object; };
private:
  std::string m_raw_response;
  std::string m_error_msg;
  T m_result_object;
};

struct Order {
  Order() {

  }

  Order(const std::string& id) : m_id(id) {

  }

  inline const std::string& GetId() const { return m_id; }
  std::string m_id;

  friend std::ostream &operator<<(std::ostream &os, const Order &res);
};

std::ostream &operator<<(std::ostream &os, const Order &res) {
  os << res.m_id;
  return os;
}

struct AccountBalance {
  AccountBalance() {

  }

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
  virtual std::string GetExchange() = 0;
  virtual Result<Order> MarketOrder(const std::string& symbol, Side side, double qty) = 0;
  virtual Result<Order> LimitOrder(const std::string& symbol, Side side, double qty, double price) = 0;
  virtual Result<AccountBalance> GetAccountBalance() = 0;
  virtual Result<std::vector<Order>> GetOpenOrders(const std::string& symbol) = 0;
  virtual void CancelAllOrders() = 0;
};