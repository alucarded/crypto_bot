#pragma once

#include "model/account_balance.h"
#include "model/order.h"
#include "model/symbol.h"

#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// TODO: maybe instead of this class use a simple std::optional
// We do not need raw response or error message at this point
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

  inline const T& Get() const { return m_result_object.value(); };
  inline T& Get() { return m_result_object.value(); };
private:
  std::string m_raw_response;
  std::string m_error_msg;
  std::optional<T> m_result_object;
};

class ExchangeClient {
public:
  virtual std::string GetExchange() = 0;
  // TODO: use it to refresh connection (have connection ready)
  // TODO: only one thread can use client now
  //virtual Result<bool> Ping() = 0;
  virtual Result<Order> MarketOrder(SymbolPairId symbol, Side side, double qty) = 0;
  virtual Result<Order> LimitOrder(SymbolPairId symbol, Side side, double qty, double price) = 0;
  virtual Result<Order> SendOrder(const Order& order) = 0;
  virtual Result<AccountBalance> GetAccountBalance() = 0;
  virtual Result<std::vector<Order>> GetOpenOrders() = 0;
  virtual Result<bool> CancelOrder(const Order& order) = 0;
  virtual void CancelAllOrders() = 0;
};