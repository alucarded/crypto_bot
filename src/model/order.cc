#include "order.h"

std::ostream& operator<< (std::ostream& os, OrderType type) {
  switch (type) {
    case OrderType::MARKET: return os << "MARKET";
    case OrderType::LIMIT: return os << "LIMIT";
    case OrderType::UNKNOWN: return os << "UNKNOWN";
    default: return os << "OrderType{" << std::to_string(int(type)) << "}";
  }
}

std::ostream& operator<< (std::ostream& os, OrderStatus status) {
  switch (status) {
    case OrderStatus::NEW: return os << "NEW";
    case OrderStatus::PARTIALLY_FILLED: return os << "PARTIALLY_FILLED";
    case OrderStatus::FILLED: return os << "FILLED";
    case OrderStatus::CANCELED: return os << "CANCELED";
    case OrderStatus::PENDING_CANCEL: return os << "PENDING_CANCEL";
    case OrderStatus::REJECTED: return os << "REJECTED";
    case OrderStatus::EXPIRED: return os << "EXPIRED";
    case OrderStatus::UNKNOWN: return os << "UNKNOWN";
    default: return os << "OrderStatus{" << std::to_string(int(status)) << "}";
  }
}

std::ostream &operator<<(std::ostream &os, const Order &res) {
  os << "{\"id\": \"" << res.m_id
    << "\", \"client_id\": \"" << res.m_client_id
    << "\", \"symbol_id\": \"" << res.m_symbol_id
    << "\", \"side\": \"" << (res.m_side == Side::BUY ? "BUY" : "SELL")
    << "\", \"order_type\": \"" << res.m_order_type
    << "\", \"quantity\": \"" << std::to_string(res.m_quantity)
    << "\", \"price\": \"" << std::to_string(res.m_price)
    << "\", \"status\": \"" << res.m_status
    << "\"}";
  return os;
}