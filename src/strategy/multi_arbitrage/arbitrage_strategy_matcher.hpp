#include "ticker.h"

#include <map>
#include <optional>

struct ExchangeParams {
  ExchangeParams(const std::string& exchange_name, double slippage, double fee)
    : m_exchange_name(exchange_name), m_slippage(slippage), m_fee(fee) {

  }

  std::string m_exchange_name;
  double m_slippage;
  double m_fee;
};

static std::map<std::string, ExchangeParams> g_exchange_params = {
  { "binance", ExchangeParams("binance", 10.0, 0.1) }
  // TODO:
};

struct ArbitrageStrategyMatch {
  ArbitrageStrategyMatch(const Ticker& best_bid, const Ticker& best_ask)
    : m_best_bid(best_bid), m_best_ask(best_ask) 
  {}
  const Ticker& m_best_bid;
  const Ticker& m_best_ask;
};

class ArbitrageStrategyMatcher {
public:
  ArbitrageStrategyMatcher() : ArbitrageStrategyMatcher(g_exchange_params){

  }

  ArbitrageStrategyMatcher(const std::map<std::string, ExchangeParams>& exchange_params) : m_exchange_params(exchange_params) {

  }

  std::optional<ArbitrageStrategyMatch> FindMatch(const std::map<std::string, Ticker>& tickers) {
    std::string best_bid_ex, best_ask_ex;
    double best_bid, best_ask;
    for (auto it = tickers.begin(); it != tickers.end(); ++it) {
      // TODO: perhaps set some constraints on volume
      if (best_ask > it->second.m_ask) {
        best_ask = it->second.m_ask;
        best_ask_ex = it->second.m_exchange;
      }
      if (best_bid < it->second.m_bid) {
        best_bid = it->second.m_bid;
        best_bid_ex = it->second.m_exchange;
      }
    }
    // TODO: check if it is a good match
    ArbitrageStrategyMatch match{tickers.at(best_bid_ex), tickers.at(best_ask_ex)};
    if (IsGoodMatch(match)) {
      return std::optional<ArbitrageStrategyMatch>{match};
    }
    return std::nullopt;
  }

private:

  bool IsGoodMatch(const ArbitrageStrategyMatch& match) {
    return false;
  }

  std::map<std::string, ExchangeParams> m_exchange_params;
};