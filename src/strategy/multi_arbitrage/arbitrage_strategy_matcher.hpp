#include "model/options.h"
#include "model/ticker.h"

#include <boost/log/trivial.hpp>

#include <cassert>
#include <optional>
#include <map>
#include <unordered_map>

static std::unordered_map<std::string, ExchangeParams> g_exchange_params = {
  { "binance", ExchangeParams("binance", 10.0, 0.001, 1) },
  { "kraken", ExchangeParams("kraken", 10.0, 0.002, 1) },
  { "bitbay", ExchangeParams("bitbay", 10.0, 0.001, 1) },
  { "poloniex", ExchangeParams("poloniex", 10.0, 0.00125, 1) },
  { "huobiglobal", ExchangeParams("huobiglobal", 10.0, 0.002, 1) },
  { "ftx", ExchangeParams("ftx", 10.0, 0.0007, 1) }
  // TODO:
};

struct ArbitrageStrategyMatch {
  ArbitrageStrategyMatch(const Ticker& best_bid, const Ticker& best_ask, double profit)
    : m_best_bid(best_bid), m_best_ask(best_ask), m_profit(profit)
  {}
  Ticker m_best_bid;
  Ticker m_best_ask;
  double m_profit;

  friend std::ostream &operator<<(std::ostream &os, const ArbitrageStrategyMatch &match);
};

std::ostream &operator<<(std::ostream &os, const ArbitrageStrategyMatch &match) {
  // TODO: print every object as JSON! Use variable names without "m_" prefix
  os << "ARBITRAGE MATCH" << std::endl << "BEST BID: " << match.m_best_bid << std::endl
      << "BEST ASK: " << std::endl << match.m_best_ask << std::endl
      << "ESTIMATED PROFIT PER BASE UNIT: " << std::to_string(match.m_profit) << std::endl;
  return os;
}

class ArbitrageStrategyMatcher {
public:
  ArbitrageStrategyMatcher() : ArbitrageStrategyMatcher(g_exchange_params, 0){

  }

  ArbitrageStrategyMatcher(const std::unordered_map<std::string, ExchangeParams>& exchange_params, double profit_margin) : m_exchange_params(exchange_params),
      m_profit_margin(profit_margin) {

  }

  virtual ~ArbitrageStrategyMatcher() {

  }

  virtual std::optional<ArbitrageStrategyMatch> FindMatch(const std::map<std::string, Ticker>& tickers) {
    if (tickers.empty()) {
      BOOST_LOG_TRIVIAL(error) << "Empty tickers map!";
      return std::nullopt;
    }
    const auto& symbol = tickers.begin()->second.m_symbol();
    std::string best_bid_ex, best_ask_ex;
    double best_bid = 0, best_ask = std::numeric_limits<double>::max();
    for (auto it = tickers.begin(); it != tickers.end(); ++it) {
      assert(symbol == it->second.m_symbol);
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
    auto best_match = ArbitrageStrategyMatch(best_bid_ticker, best_ask_ticker, profit);
    // auto bit = m_best_matches.find(symbol);
    // if (bit == m_best_matches.end()) {
    //   m_best_matches.emplace(symbol, best_match);
    // } else {
    //   const auto& prev_match = m_best_matches.at(symbol);
    //   if (prev_match.m_profit < best_match.m_profit) {
    //     bit->second = best_match;
    //     BOOST_LOG_TRIVIAL(debug) << "Best match for " << symbol << " updated";
    //     BOOST_LOG_TRIVIAL(debug) << best_match;
    //   }
    // }
    if (profit >= 0) {
      return std::optional<ArbitrageStrategyMatch>{best_match};
    }
    return std::nullopt;
  }

private:

  double CalculateProfit(const Ticker& best_bid_ticker, const Ticker& best_ask_ticker) {
    // TODO: perhaps add some validations and sanity checks eg. this is not the same exchange on both bid and ask side
    if (0 == m_exchange_params.count(best_bid_ticker.m_exchange)
      || 0 == m_exchange_params.count(best_ask_ticker.m_exchange)) {
        return std::numeric_limits<double>::lowest();
    }
    const auto& bid_side_params = m_exchange_params.at(best_bid_ticker.m_exchange);
    const auto& ask_side_params = m_exchange_params.at(best_ask_ticker.m_exchange);
    return (1 - bid_side_params.fee - m_profit_margin)*(best_bid_ticker.m_bid - bid_side_params.slippage) - (1 + ask_side_params.fee + m_profit_margin)*(best_ask_ticker.m_ask + ask_side_params.slippage);
  }

private:
  std::unordered_map<std::string, ExchangeParams> m_exchange_params;
  double m_profit_margin;
};