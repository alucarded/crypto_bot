#pragma once

#include <iostream>
#include <string>
#include <unordered_map>

enum SymbolPairId : int {
  ADA_USDT = 0,
  BTC_USDT,
  ETH_USDT,
  EOS_USDT,
  ADA_BTC,
  ETH_BTC,
  EOS_BTC,
  EOS_ETH,
  UNKNOWN
};

std::ostream& operator<< (std::ostream& os, SymbolPairId spid) {
  switch (spid) {
    case SymbolPairId::ADA_USDT: return os << "ADA_USDT";
    case SymbolPairId::BTC_USDT: return os << "BTC_USDT";
    case SymbolPairId::ETH_USDT: return os << "ETH_USDT";
    case SymbolPairId::EOS_USDT: return os << "EOS_USDT";
    case SymbolPairId::ADA_BTC: return os << "ADA_BTC";
    case SymbolPairId::ETH_BTC: return os << "ETH_BTC";
    case SymbolPairId::EOS_BTC: return os << "EOS_BTC";
    case SymbolPairId::EOS_ETH: return os << "EOS_ETH";
    default: return os << "SymbolPairId{" << std::to_string(int(spid)) << "}";
  }
}

enum SymbolId : int {
  ADA = 0,
  BTC,
  ETH,
  EOS,
  USDT
};

class SymbolPair {
public:
  SymbolPair() : m_symbol_id(SymbolPairId::UNKNOWN) {}
  SymbolPair(const std::string& s) : SymbolPair(GetSymbol(s)) {}
  SymbolPair(SymbolPairId id) : m_symbol_id(id) {}
  SymbolPair& operator=(const std::string& s) {
    m_symbol_id = GetSymbol(s);
    return *this;
  }
  operator SymbolPairId() const { return m_symbol_id; }
  const SymbolPairId& operator()() const { return m_symbol_id; }

  using AssetMap = std::unordered_map<SymbolPairId, SymbolId>;
  SymbolId GetBaseAsset() const {
    static const AssetMap s_base_map = {
      {SymbolPairId::ADA_USDT, SymbolId::ADA},
      {SymbolPairId::BTC_USDT, SymbolId::BTC},
      {SymbolPairId::ETH_USDT, SymbolId::ETH},
      {SymbolPairId::EOS_USDT, SymbolId::EOS},
      {SymbolPairId::ADA_BTC, SymbolId::ADA},
      {SymbolPairId::EOS_BTC, SymbolId::EOS},
      {SymbolPairId::EOS_ETH, SymbolId::EOS},
    };
    return s_base_map.at(m_symbol_id);
  }

  SymbolId GetQuoteAsset() const {
    static const AssetMap s_quote_map = {
      {SymbolPairId::ADA_USDT, SymbolId::USDT},
      {SymbolPairId::BTC_USDT, SymbolId::USDT},
      {SymbolPairId::ETH_USDT, SymbolId::USDT},
      {SymbolPairId::EOS_USDT, SymbolId::USDT},
      {SymbolPairId::ADA_BTC, SymbolId::BTC},
      {SymbolPairId::EOS_BTC, SymbolId::BTC},
      {SymbolPairId::EOS_ETH, SymbolId::ETH},
    };
    return s_quote_map.at(m_symbol_id);
  }
private:
  static SymbolPairId GetSymbol(const std::string& s) {
    auto it = getSymbols().find(s);
    if (it == getSymbols().end()) {
      throw std::invalid_argument(s + " has no associated SymbolPairId value");
    }
    return it->second;
  }

  using SymbolPairMap = std::unordered_map<std::string, SymbolPairId>;
  static const SymbolPairMap& getSymbols() {
    // TODO: source-specific strings should be in source-specific classes
    static const SymbolPairMap ret{
      {"XBT/USDT", SymbolPairId::BTC_USDT},
      {"BTCUSDT", SymbolPairId::BTC_USDT},
      {"ETH/USDT", SymbolPairId::ETH_USDT},
      {"ETHUSDT", SymbolPairId::ETH_USDT},
      {"ADA/USDT", SymbolPairId::ADA_USDT},
      {"ADAUSDT", SymbolPairId::ADA_USDT},
      {"EOS/USDT", SymbolPairId::EOS_USDT},
      {"EOSUSDT", SymbolPairId::EOS_USDT},
      {"ETH/XBT", SymbolPairId::ETH_BTC},
      {"ETHBTC", SymbolPairId::ETH_BTC},
      {"ADA/XBT", SymbolPairId::ADA_BTC},
      {"ADABTC", SymbolPairId::ADA_BTC},
      {"EOS/XBT", SymbolPairId::EOS_BTC},
      {"EOSBTC", SymbolPairId::EOS_BTC},
      {"EOS/ETH", SymbolPairId::EOS_ETH},
      {"EOSETH", SymbolPairId::EOS_ETH}
    };
    return ret;
  }

private:
  SymbolPairId m_symbol_id;
};
