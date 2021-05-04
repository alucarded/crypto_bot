#include "symbol.h"

std::ostream& operator<< (std::ostream& os, SymbolPairId spid) {
  switch (spid) {
    case SymbolPairId::ADA_USDT: return os << "ADA_USDT";
    case SymbolPairId::BNB_USDT: return os << "BNB_USDT";
    case SymbolPairId::BTC_USDT: return os << "BTC_USDT";
    case SymbolPairId::DOGE_USDT: return os << "DOGE_USDT";
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

std::ostream& operator<< (std::ostream& os, SymbolId sid) {
  switch (sid) {
    case SymbolId::ADA: return os << "ADA";
    case SymbolId::BNB: return os << "BNB";
    case SymbolId::BTC: return os << "BTC";
    case SymbolId::DOGE: return os << "DOGE";
    case SymbolId::DOT: return os << "DOT";
    case SymbolId::ETH: return os << "ETH";
    case SymbolId::EOS: return os << "EOS";
    case SymbolId::USDT: return os << "USDT";
    case SymbolId::XLM: return os << "XLM";
    case SymbolId::UNKNOWN_ASSET: return os << "UNKNOWN_ASSET";
    default: return os << "SymbolPairId{" << std::to_string(int(sid)) << "}";
  }
}

const std::unordered_map<SymbolPairId, std::string> BINANCE_SYMBOL_TO_STRING_MAP = {
  {SymbolPairId::ADA_USDT, "ADAUSDT"},
  {SymbolPairId::BNB_USDT, "BNBUSDT"},
  {SymbolPairId::BTC_USDT, "BTCUSDT"},
  {SymbolPairId::DOGE_USDT, "DOGEUSDT"},
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

const std::unordered_map<std::string, SymbolId> BINANCE_ASSET_MAP = {
  {"ADA", SymbolId::ADA},
  {"BNB", SymbolId::BNB},
  {"BTC", SymbolId::BTC},
  {"DOGE", SymbolId::DOGE},
  {"DOT", SymbolId::DOT},
  {"ETH", SymbolId::ETH},
  {"EOS", SymbolId::EOS},
  {"USDT", SymbolId::USDT},
  {"XLM", SymbolId::XLM}
};

const std::unordered_map<SymbolPairId, std::string> KRAKEN_SYMBOL_TO_STRING_MAP = {
  {SymbolPairId::ADA_USDT, "ADAUSDT"},
  {SymbolPairId::BTC_USDT, "BTCUSDT"},
  {SymbolPairId::DOGE_USDT, "DOGEUSDT"},
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

const std::unordered_map<std::string, SymbolId> KRAKEN_ASSET_MAP = {
  {"ADA", SymbolId::ADA},
  {"DOT", SymbolId::DOT},
  {"XBT", SymbolId::BTC},
  {"XXBT", SymbolId::BTC},
  {"XDG", SymbolId::DOGE},
  {"XXDG", SymbolId::DOGE},
  {"ETH", SymbolId::ETH},
  {"XETH", SymbolId::ETH},
  {"EOS", SymbolId::EOS},
  {"USDT", SymbolId::USDT},
  {"XXLM", SymbolId::XLM},
};