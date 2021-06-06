#include "model/options.h"
#include "model/ticker.h"

#include <boost/log/trivial.hpp>

#include <cassert>
#include <optional>
#include <map>
#include <unordered_map>

// static std::unordered_map<std::string, ExchangeParams> g_exchange_params = {
//   { "binance", ExchangeParams("binance", 10.0, 0.001, 1) },
//   { "kraken", ExchangeParams("kraken", 10.0, 0.002, 1) },
//   { "bitbay", ExchangeParams("bitbay", 10.0, 0.001, 1) },
//   { "poloniex", ExchangeParams("poloniex", 10.0, 0.00125, 1) },
//   { "huobiglobal", ExchangeParams("huobiglobal", 10.0, 0.002, 1) },
//   { "ftx", ExchangeParams("ftx", 10.0, 0.0007, 1) }
//   // TODO:
// };

struct ArbitrageStrategyMatch {
  ArbitrageStrategyMatch(const Ticker& best_bid_, const Ticker& best_ask_, double profit_)
    : best_bid(best_bid_), best_ask(best_ask_), profit(profit_)
  {}
  Ticker best_bid;
  Ticker best_ask;
  double profit;

  friend std::ostream &operator<<(std::ostream &os, const ArbitrageStrategyMatch &match);
};

std::ostream &operator<<(std::ostream &os, const ArbitrageStrategyMatch &match);

class ArbitrageStrategyMatcher {
public:
  ArbitrageStrategyMatcher(const std::unordered_map<std::string, ExchangeParams>& exchange_params, double profit_margin);

  virtual ~ArbitrageStrategyMatcher();

  virtual std::optional<ArbitrageStrategyMatch> FindMatch(const std::map<std::string, Ticker>& tickers);

private:
  double CalculateProfit(const Ticker& best_bid_ticker, const Ticker& best_ask_ticker) const;

private:
  std::unordered_map<std::string, ExchangeParams> m_exchange_params;
  double m_profit_margin;
};