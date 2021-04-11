#pragma once

#include <iostream>
#include <string>
#include <unordered_map>

enum SymbolPairId : int {
  ADA_USDT = 0,
  BNB_USDT,
  BTC_USDT,
  DOT_USDT,
  ETH_USDT,
  EOS_USDT,
  ADA_BTC,
  BNB_BTC,
  DOT_BTC,
  ETH_BTC,
  EOS_BTC,
  EOS_ETH,
  XLM_BTC,
  UNKNOWN
};

std::ostream& operator<< (std::ostream& os, SymbolPairId spid) {
  switch (spid) {
    case SymbolPairId::ADA_USDT: return os << "ADA_USDT";
    case SymbolPairId::BNB_USDT: return os << "BNB_USDT";
    case SymbolPairId::BTC_USDT: return os << "BTC_USDT";
    case SymbolPairId::DOT_USDT: return os << "DOT_USDT";
    case SymbolPairId::ETH_USDT: return os << "ETH_USDT";
    case SymbolPairId::EOS_USDT: return os << "EOS_USDT";
    case SymbolPairId::ADA_BTC: return os << "ADA_BTC";
    case SymbolPairId::BNB_BTC: return os << "BNB_BTC";
    case SymbolPairId::DOT_BTC: return os << "DOT_BTC";
    case SymbolPairId::ETH_BTC: return os << "ETH_BTC";
    case SymbolPairId::EOS_BTC: return os << "EOS_BTC";
    case SymbolPairId::EOS_ETH: return os << "EOS_ETH";
    case SymbolPairId::XLM_BTC: return os << "XLM_BTC";
    case SymbolPairId::UNKNOWN: return os << "UNKNOWN";
    default: return os << "SymbolPairId{" << std::to_string(int(spid)) << "}";
  }
}

enum SymbolId : int {
  ADA = 0,
  BNB,
  BTC,
  DOT,
  ETH,
  EOS,
  USDT,
  XLM,
  UNKNOWN_ASSET
};

std::ostream& operator<< (std::ostream& os, SymbolId sid) {
  switch (sid) {
    case SymbolId::ADA: return os << "ADA";
    case SymbolId::BNB: return os << "BNB";
    case SymbolId::BTC: return os << "BTC";
    case SymbolId::DOT: return os << "DOT";
    case SymbolId::ETH: return os << "ETH";
    case SymbolId::EOS: return os << "EOS";
    case SymbolId::USDT: return os << "USDT";
    case SymbolId::XLM: return os << "XLM";
    case SymbolId::UNKNOWN_ASSET: return os << "UNKNOWN_ASSET";
    default: return os << "SymbolPairId{" << std::to_string(int(sid)) << "}";
  }
}

class SymbolPair {
public:
  SymbolPair() : m_symbol_id(SymbolPairId::UNKNOWN) {}
  SymbolPair(SymbolPairId id) : m_symbol_id(id) {}

  operator SymbolPairId() const { return m_symbol_id; }
  const SymbolPairId& operator()() const { return m_symbol_id; }

  static SymbolPair FromBinanceString(const std::string& s) {
    return SymbolPair(GetBinanceSymbol(s));
  }

  static SymbolPair FromKrakenString(const std::string& s) {
    return SymbolPair(GetKrakenSymbol(s));
  }

  using AssetMap = std::unordered_map<SymbolPairId, SymbolId>;
  SymbolId GetBaseAsset() const {
    static const AssetMap s_base_map = {
      {SymbolPairId::ADA_USDT, SymbolId::ADA},
      {SymbolPairId::BNB_USDT, SymbolId::BNB},
      {SymbolPairId::BTC_USDT, SymbolId::BTC},
      {SymbolPairId::DOT_USDT, SymbolId::DOT},
      {SymbolPairId::ETH_USDT, SymbolId::ETH},
      {SymbolPairId::EOS_USDT, SymbolId::EOS},
      {SymbolPairId::ADA_BTC, SymbolId::ADA},
      {SymbolPairId::BNB_BTC, SymbolId::BNB},
      {SymbolPairId::DOT_BTC, SymbolId::DOT},
      {SymbolPairId::EOS_BTC, SymbolId::EOS},
      {SymbolPairId::EOS_ETH, SymbolId::EOS},
      {SymbolPairId::ETH_BTC, SymbolId::ETH},
      {SymbolPairId::XLM_BTC, SymbolId::XLM},
    };
    return s_base_map.at(m_symbol_id);
  }

  SymbolId GetQuoteAsset() const {
    static const AssetMap s_quote_map = {
      {SymbolPairId::ADA_USDT, SymbolId::USDT},
      {SymbolPairId::BNB_USDT, SymbolId::USDT},
      {SymbolPairId::BTC_USDT, SymbolId::USDT},
      {SymbolPairId::DOT_USDT, SymbolId::USDT},
      {SymbolPairId::ETH_USDT, SymbolId::USDT},
      {SymbolPairId::EOS_USDT, SymbolId::USDT},
      {SymbolPairId::ADA_BTC, SymbolId::BTC},
      {SymbolPairId::BNB_BTC, SymbolId::BTC},
      {SymbolPairId::DOT_BTC, SymbolId::BTC},
      {SymbolPairId::EOS_BTC, SymbolId::BTC},
      {SymbolPairId::EOS_ETH, SymbolId::ETH},
      {SymbolPairId::ETH_BTC, SymbolId::BTC},
      {SymbolPairId::XLM_BTC, SymbolId::BTC},
    };
    return s_quote_map.at(m_symbol_id);
  }
private:
  static SymbolPairId GetBinanceSymbol(const std::string& s) {
    auto it = getBinanceSymbols().find(s);
    if (it == getBinanceSymbols().end()) {
      return SymbolPairId::UNKNOWN;
    }
    return it->second;
  }

  static SymbolPairId GetKrakenSymbol(const std::string& s) {
    auto it = getKrakenSymbols().find(s);
    if (it == getKrakenSymbols().end()) {
      return SymbolPairId::UNKNOWN;
    }
    return it->second;
  }

  using SymbolPairMap = std::unordered_map<std::string, SymbolPairId>;
  static const SymbolPairMap& getBinanceSymbols() {
    static const SymbolPairMap ret{
      {"ADAUSDT", SymbolPairId::ADA_USDT},
      {"BNBUSDT", SymbolPairId::BNB_USDT},
      {"BTCUSDT", SymbolPairId::BTC_USDT},
      {"DOTUSDT", SymbolPairId::DOT_USDT},
      {"ETHUSDT", SymbolPairId::ETH_USDT},
      {"EOSUSDT", SymbolPairId::EOS_USDT},
      {"ADABTC", SymbolPairId::ADA_BTC},
      {"BNBBTC", SymbolPairId::BNB_BTC},
      {"DOTBTC", SymbolPairId::DOT_BTC},
      {"ETHBTC", SymbolPairId::ETH_BTC},
      {"EOSBTC", SymbolPairId::EOS_BTC},
      {"EOSETH", SymbolPairId::EOS_ETH},
      {"XLMBTC", SymbolPairId::XLM_BTC},
    };
    return ret;
  }

  static const SymbolPairMap& getKrakenSymbols() {
    static const SymbolPairMap ret{
      {"ADA/USDT", SymbolPairId::ADA_USDT},
      {"ADAUSDT", SymbolPairId::ADA_USDT},
      {"XBT/USDT", SymbolPairId::BTC_USDT},
      {"XBTUSDT", SymbolPairId::BTC_USDT},
      {"DOT/USDT", SymbolPairId::DOT_USDT},
      {"DOTUSDT", SymbolPairId::DOT_USDT},
      {"ETH/USDT", SymbolPairId::ETH_USDT},
      {"ETHUSDT", SymbolPairId::ETH_USDT},
      {"EOS/USDT", SymbolPairId::EOS_USDT},
      {"EOSUSDT", SymbolPairId::EOS_USDT},
      {"ETH/XBT", SymbolPairId::ETH_BTC},
      {"ETHXBT", SymbolPairId::ETH_BTC},
      {"ADA/XBT", SymbolPairId::ADA_BTC},
      {"ADAXBT", SymbolPairId::ADA_BTC},
      {"DOT/XBT", SymbolPairId::DOT_BTC},
      {"DOTXBT", SymbolPairId::DOT_BTC},
      {"EOS/XBT", SymbolPairId::EOS_BTC},
      {"EOSXBT", SymbolPairId::EOS_BTC},
      {"EOS/ETH", SymbolPairId::EOS_ETH},
      {"EOSETH", SymbolPairId::EOS_ETH},
      {"XLM/XBT", SymbolPairId::XLM_BTC},
      {"XLMXBT", SymbolPairId::XLM_BTC},
    };
    return ret;
  }

private:
  SymbolPairId m_symbol_id;
};

extern const std::unordered_map<SymbolPairId, std::string> BINANCE_SYMBOL_TO_STRING_MAP = {
  {SymbolPairId::ADA_USDT, "ADAUSDT"},
  {SymbolPairId::BNB_USDT, "BNBUSDT"},
  {SymbolPairId::BTC_USDT, "BTCUSDT"},
  {SymbolPairId::DOT_USDT, "DOTUSDT"},
  {SymbolPairId::ETH_USDT, "ETHUSDT"},
  {SymbolPairId::EOS_USDT, "EOSUSDT"},
  {SymbolPairId::ADA_BTC, "ADABTC"},
  {SymbolPairId::BNB_BTC, "BNBBTC"},
  {SymbolPairId::DOT_BTC, "DOTBTC"},
  {SymbolPairId::ETH_BTC, "ETHBTC"},
  {SymbolPairId::EOS_BTC, "EOSBTC"},
  {SymbolPairId::EOS_ETH, "EOSETH"},
  {SymbolPairId::XLM_BTC, "XLMBTC"}
};

extern const std::unordered_map<std::string, SymbolId> BINANCE_ASSET_MAP = {
  {"ADA", SymbolId::ADA},
  {"BNB", SymbolId::BNB},
  {"BTC", SymbolId::BTC},
  {"DOT", SymbolId::DOT},
  {"ETH", SymbolId::ETH},
  {"EOS", SymbolId::EOS},
  {"USDT", SymbolId::USDT},
  {"XLM", SymbolId::XLM}
};

extern const std::unordered_map<SymbolPairId, std::string> KRAKEN_SYMBOL_TO_STRING_MAP = {
  {SymbolPairId::ADA_USDT, "ADAUSDT"},
  {SymbolPairId::BTC_USDT, "BTCUSDT"},
  {SymbolPairId::DOT_USDT, "DOTUSDT"},
  {SymbolPairId::ETH_USDT, "ETHUSDT"},
  {SymbolPairId::EOS_USDT, "EOSUSDT"},
  {SymbolPairId::ADA_BTC, "ADAXBT"},
  {SymbolPairId::DOT_BTC, "DOTXBT"},
  {SymbolPairId::ETH_BTC, "ETHXBT"},
  {SymbolPairId::EOS_BTC, "EOSXBT"},
  {SymbolPairId::EOS_ETH, "EOSETH"},
  {SymbolPairId::XLM_BTC, "XLMXBT"},
};

extern const std::unordered_map<std::string, SymbolId> KRAKEN_ASSET_MAP = {
  {"ADA", SymbolId::ADA},
  {"DOT", SymbolId::DOT},
  {"XBT", SymbolId::BTC},
  {"XXBT", SymbolId::BTC},
  {"ETH", SymbolId::ETH},
  {"XETH", SymbolId::ETH},
  {"EOS", SymbolId::EOS},
  {"USDT", SymbolId::USDT},
  {"XXLM", SymbolId::XLM},
};