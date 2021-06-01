#pragma once

#include <iostream>
#include <string>
#include <unordered_map>

enum SymbolPairId : int {
  ADA_USDT = 0,
  BNB_USDT,
  BTC_USDT,
  DOGE_USDT,
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

std::ostream& operator<< (std::ostream& os, SymbolPairId spid);

enum SymbolId : int {
  ADA = 0,
  BNB,
  BTC,
  DOGE,
  DOT,
  ETH,
  EOS,
  USDT,
  XLM,
  UNKNOWN_ASSET
};

std::ostream& operator<< (std::ostream& os, SymbolId sid);

struct SymbolPairKey {
  SymbolPairKey(SymbolId base_id_, SymbolId quote_id_) : base_id(base_id_), quote_id(quote_id_) {

  }

  bool operator<(const SymbolPairKey& o) const {
    if (base_id != o.base_id) {
      return base_id < o.base_id;
    }
    return quote_id < o.quote_id;
  }

  bool operator==(const SymbolPairKey& o) const {
    return base_id == o.base_id && quote_id == o.quote_id;
  }

  SymbolId base_id;
  SymbolId quote_id;
};

namespace std {

  template <>
  struct hash<SymbolPairKey>
  {
    std::size_t operator()(const SymbolPairKey& k) const
    {
      using std::size_t;
      using std::hash;

      return hash<int>()(k.base_id) ^ (hash<int>()(k.quote_id) << 1);
    }
  };

}

class SymbolPair {
public:
  SymbolPair() : m_symbol_id(SymbolPairId::UNKNOWN) {}
  SymbolPair(SymbolPairId id) : m_symbol_id(id) {}
  SymbolPair(SymbolId base_id, SymbolId quote_id) : m_symbol_id(GetSymbolPair(base_id, quote_id)) {}
  SymbolPair(const std::string& str) : m_symbol_id(GetStringSymbol(str)) {}

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
      {SymbolPairId::DOGE_USDT, SymbolId::DOGE},
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
      {SymbolPairId::DOGE_USDT, SymbolId::USDT},
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

  static SymbolPairId GetStringSymbol(const std::string& s) {
    auto it = getStringSymbols().find(s);
    if (it == getStringSymbols().end()) {
      return SymbolPairId::UNKNOWN;
    }
    return it->second;
  }

  static SymbolPairId GetSymbolPair(SymbolId base_id, SymbolId quote_id) {
    static const std::unordered_map<SymbolPairKey, SymbolPairId> s_symbol_pair_map{
      {SymbolPairKey(SymbolId::ADA, SymbolId::USDT), SymbolPairId::ADA_USDT},
      {SymbolPairKey(SymbolId::BNB, SymbolId::USDT), SymbolPairId::BNB_USDT},
      {SymbolPairKey(SymbolId::BTC, SymbolId::USDT), SymbolPairId::BTC_USDT},
      {SymbolPairKey(SymbolId::DOGE, SymbolId::USDT), SymbolPairId::DOGE_USDT},
      {SymbolPairKey(SymbolId::DOT, SymbolId::USDT), SymbolPairId::DOT_USDT},
      {SymbolPairKey(SymbolId::ETH, SymbolId::USDT), SymbolPairId::ETH_USDT},
      {SymbolPairKey(SymbolId::EOS, SymbolId::USDT), SymbolPairId::EOS_USDT},
      {SymbolPairKey(SymbolId::ADA, SymbolId::BTC), SymbolPairId::ADA_BTC},
      {SymbolPairKey(SymbolId::BNB, SymbolId::BTC), SymbolPairId::BNB_BTC},
      {SymbolPairKey(SymbolId::DOT, SymbolId::BTC), SymbolPairId::DOT_BTC},
      {SymbolPairKey(SymbolId::ETH, SymbolId::BTC), SymbolPairId::ETH_BTC},
      {SymbolPairKey(SymbolId::EOS, SymbolId::BTC), SymbolPairId::EOS_BTC},
      {SymbolPairKey(SymbolId::EOS, SymbolId::ETH), SymbolPairId::EOS_ETH},
      {SymbolPairKey(SymbolId::XLM, SymbolId::BTC), SymbolPairId::XLM_BTC},
    };
    return s_symbol_pair_map.at(SymbolPairKey(base_id, quote_id));
  }

  using SymbolPairMap = std::unordered_map<std::string, SymbolPairId>;

  static const SymbolPairMap& getBinanceSymbols() {
    static const SymbolPairMap ret{
      {"ADAUSDT", SymbolPairId::ADA_USDT},
      {"BNBUSDT", SymbolPairId::BNB_USDT},
      {"BTCUSDT", SymbolPairId::BTC_USDT},
      {"DOGEUSDT", SymbolPairId::DOGE_USDT},
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
      {"XDG/USDT", SymbolPairId::DOGE_USDT},
      {"XDGUSDT", SymbolPairId::DOGE_USDT},
      {"DOGE/USDT", SymbolPairId::DOGE_USDT},
      {"DOGEUSDT", SymbolPairId::DOGE_USDT},
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

  static const SymbolPairMap& getStringSymbols() {
    static const SymbolPairMap ret{
      {"ADA_USDT", SymbolPairId::ADA_USDT},
      {"BNB_USDT", SymbolPairId::BNB_USDT},
      {"BTC_USDT", SymbolPairId::BTC_USDT},
      {"DOGE_USDT", SymbolPairId::DOGE_USDT},
      {"DOT_USDT", SymbolPairId::DOT_USDT},
      {"ETH_USDT", SymbolPairId::ETH_USDT},
      {"EOS_USDT", SymbolPairId::EOS_USDT},
      {"ADA_BTC", SymbolPairId::ADA_BTC},
      {"BNB_BTC", SymbolPairId::BNB_BTC},
      {"DOT_BTC", SymbolPairId::DOT_BTC},
      {"ETH_BTC", SymbolPairId::ETH_BTC},
      {"EOS_BTC", SymbolPairId::EOS_BTC},
      {"EOS_ETH", SymbolPairId::EOS_ETH},
      {"XLM_BTC", SymbolPairId::XLM_BTC},
    };
    return ret;
  }

private:
  SymbolPairId m_symbol_id;
};

extern const std::unordered_map<SymbolPairId, std::string> BINANCE_SYMBOL_TO_STRING_MAP;

extern const std::unordered_map<std::string, SymbolId> BINANCE_ASSET_MAP;

extern const std::unordered_map<SymbolPairId, std::string> KRAKEN_SYMBOL_TO_STRING_MAP;

extern const std::unordered_map<std::string, SymbolId> KRAKEN_ASSET_MAP;