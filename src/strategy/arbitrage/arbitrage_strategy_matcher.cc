
#include "arbitrage_strategy_matcher.h"

std::ostream &operator<<(std::ostream &os, const ArbitrageStrategyMatch &match) {
  // TODO: print every object as JSON! Use variable names without "m_" prefix
  os << "ARBITRAGE MATCH" << std::endl << "BEST BID: " << match.best_bid << std::endl
      << "BEST ASK: " << std::endl << match.best_ask << std::endl
      << "ESTIMATED PROFIT PER BASE UNIT: " << std::to_string(match.profit) << std::endl;
  return os;
}

ArbitrageStrategyMatcher::ArbitrageStrategyMatcher(const std::unordered_map<std::string, ExchangeParams>& exchange_params, double profit_margin)
    : m_exchange_params(exchange_params), m_profit_margin(profit_margin) {

}

ArbitrageStrategyMatcher::~ArbitrageStrategyMatcher() {
  
}

std::optional<ArbitrageStrategyMatch> ArbitrageStrategyMatcher::FindMatch(const std::map<std::string, Ticker>& tickers) {
  if (tickers.empty()) {
    BOOST_LOG_TRIVIAL(error) << "Empty tickers map!";
    return std::nullopt;
  }
  const auto& symbol = tickers.begin()->second.symbol();
  std::string best_bid_ex, best_ask_ex;
  double best_bid = 0, best_ask = std::numeric_limits<double>::max();
  for (auto it = tickers.begin(); it != tickers.end(); ++it) {
    assert(symbol == it->second.symbol);
    // TODO: perhaps set some constraints on volume
    if (best_ask > it->second.ask) {
      best_ask = it->second.ask;
      best_ask_ex = it->second.exchange;
    }
    if (best_bid < it->second.bid) {
      best_bid = it->second.bid;
      best_bid_ex = it->second.exchange;
    }
  }
  if (best_bid_ex.empty() || best_ask_ex.empty()) {
    return std::nullopt;
  }
  const Ticker& best_bid_ticker = tickers.at(best_bid_ex);
  const Ticker& best_ask_ticker = tickers.at(best_ask_ex);
  double profit = CalculateProfit(best_bid_ticker, best_ask_ticker);
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
    // BOOST_LOG_TRIVIAL(debug) << "best bid: " << best_bid_ticker << ", best ask: " << best_ask_ticker;
    // BOOST_LOG_TRIVIAL(debug) << profit;
    auto best_match = ArbitrageStrategyMatch(best_bid_ticker, best_ask_ticker, profit);
    return std::optional<ArbitrageStrategyMatch>{best_match};
  }
  return std::nullopt;
}

double ArbitrageStrategyMatcher::CalculateProfit(const Ticker& best_bid_ticker, const Ticker& best_ask_ticker) const {
  // TODO: perhaps add some validations and sanity checks eg. this is not the same exchange on both bid and ask side
  if (0 == m_exchange_params.count(best_bid_ticker.exchange)
    || 0 == m_exchange_params.count(best_ask_ticker.exchange)) {
      return std::numeric_limits<double>::lowest();
  }
  const auto& bid_side_params = m_exchange_params.at(best_bid_ticker.exchange);
  const auto& ask_side_params = m_exchange_params.at(best_ask_ticker.exchange);
  return (1 - bid_side_params.fee - m_profit_margin)*(best_bid_ticker.bid - bid_side_params.slippage) - (1 + ask_side_params.fee + m_profit_margin)*(best_ask_ticker.ask + ask_side_params.slippage);
}