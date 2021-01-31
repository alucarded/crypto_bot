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
  ArbitrageStrategyMatch(const Ticker& best_bid, const Ticker& best_ask, double profit)
    : m_best_bid(best_bid), m_best_ask(best_ask), m_profit(profit)
  {}
  const Ticker& m_best_bid;
  const Ticker& m_best_ask;
  double m_profit;
};

class ArbitrageStrategyMatcher {
public:
  ArbitrageStrategyMatcher() : ArbitrageStrategyMatcher(g_exchange_params){

  }

  ArbitrageStrategyMatcher(const std::map<std::string, ExchangeParams>& exchange_params) : m_exchange_params(exchange_params) {

  }

  std::optional<ArbitrageStrategyMatch> FindMatch(const std::map<std::string, Ticker>& tickers) {
    std::string best_bid_ex, best_ask_ex;
    double best_bid = 0, best_ask = std::numeric_limits<double>::max();
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
    if (best_bid_ex.empty() || best_ask_ex.empty()) {
      return std::nullopt;
    }
    const Ticker& best_bid_ticker = tickers.at(best_bid_ex);
    const Ticker& best_ask_ticker = tickers.at(best_ask_ex);
    double profit = CalculateProfit(best_bid_ticker, best_ask_ticker);
    if (profit >= 0) {
      return std::optional<ArbitrageStrategyMatch>{ArbitrageStrategyMatch(best_bid_ticker, best_ask_ticker, profit)};
    }
    return std::nullopt;
  }

private:

  double CalculateProfit(const Ticker& best_bid_ticker, const Ticker& best_ask_ticker) {
    // TODO: perhaps add some validations and sanity checks eg. this is not the same exchange on both bid and ask side
    const auto& bid_side_params = m_exchange_params.at(best_bid_ticker.m_exchange);
    const auto& ask_side_params = m_exchange_params.at(best_ask_ticker.m_exchange);
    return best_bid_ticker.m_bid - best_ask_ticker.m_ask - bid_side_params.m_slippage - bid_side_params.m_fee - ask_side_params.m_slippage - ask_side_params.m_fee;
  }

  std::map<std::string, ExchangeParams> m_exchange_params;
};