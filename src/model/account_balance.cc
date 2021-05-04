#include "account_balance.h"

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