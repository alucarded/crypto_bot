#include <string>
#include <unordered_map>

enum SymbolId : int {
  ADA_USDT,
  BTC_USDT,
  ETH_USDT,
  EOS_USDT,
  ADA_BTC,
  ETH_BTC,
  EOS_BTC,
  EOS_ETH,
  UNKNOWN
};

class PairSymbol {
public:
  PairSymbol() : m_symbol_id(SymbolId::UNKNOWN) {}
  PairSymbol(const std::string& s) : PairSymbol(GetSymbol(s)) {}
  PairSymbol(SymbolId id) : m_symbol_id(id) {}
  PairSymbol& operator=(const std::string& s) {
    m_symbol_id = GetSymbol(s);
    return *this;
  }
  operator SymbolId() const { return m_symbol_id; }

private:
  static SymbolId GetSymbol(const std::string& s) {
    auto it = getSymbols().find(s);
    if (it == getSymbols().end()) {
      throw std::invalid_argument(s + " has no associated SymbolId value");
    }
    return it->second;
  }

  using PairSymbolMap = std::unordered_map<std::string, SymbolId>;
  static const PairSymbolMap& getSymbols() {
    static const PairSymbolMap ret{
      {"XBT/USDT", SymbolId::BTC_USDT},
      {"BTCUSDT", SymbolId::BTC_USDT},
      {"ETH/USDT", SymbolId::ETH_USDT},
      {"ETHUSDT", SymbolId::ETH_USDT},
      {"ADA/USDT", SymbolId::ADA_USDT},
      {"ADAUSDT", SymbolId::ADA_USDT},
      {"EOS/USDT", SymbolId::EOS_USDT},
      {"EOSUSDT", SymbolId::EOS_USDT},
      {"ETH/XBT", SymbolId::ETH_BTC},
      {"ETHBTC", SymbolId::ETH_BTC},
      {"ADA/XBT", SymbolId::ADA_BTC},
      {"ADABTC", SymbolId::ADA_BTC},
      {"EOS/XBT", SymbolId::EOS_BTC},
      {"EOSBTC", SymbolId::EOS_BTC},
      {"EOS/ETH", SymbolId::EOS_ETH},
      {"EOSETH", SymbolId::EOS_ETH}
    };
    return ret;
  }

private:
  SymbolId m_symbol_id;
};
