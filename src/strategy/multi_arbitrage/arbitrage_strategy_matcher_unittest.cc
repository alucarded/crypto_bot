#include "arbitrage_strategy_matcher.hpp"

#include "gtest/gtest.h"

#include <map>
#include <unordered_map>

class ArbitrageStrategyMatcherFixture : public testing::Test { 
public:
  ArbitrageStrategyMatcherFixture() {
    // Initialization code here
    std::unordered_map<std::string, ExchangeParams> exchange_params;
    exchange_params.emplace("A", ExchangeParams("A", 0.01, 0.01, 1));
    exchange_params.emplace("B", ExchangeParams("B", 0.01, 0.02, 1));
    arbitrage_strategy_matcher = ArbitrageStrategyMatcher(exchange_params, 0);
  }

  virtual void SetUp() override {
    // no-op
  }

  virtual void TearDown() override {
    // no-op
  }

  ~ArbitrageStrategyMatcherFixture()  {
  }

protected:
  ArbitrageStrategyMatcher arbitrage_strategy_matcher;
};

TEST_F(ArbitrageStrategyMatcherFixture, OnTickerTest)
{
  Ticker ticker1;
  ticker1.m_ask = 1.1;
  ticker1.m_bid = 1.0;
  ticker1.m_exchange = "A";
  Ticker ticker2;
  ticker2.m_ask = 1.3;
  ticker2.m_bid = 1.2;
  ticker2.m_exchange = "B";
  std::map<std::string, Ticker> ticker_map = {
    { ticker1.m_exchange, ticker1 },
    { ticker2.m_exchange, ticker2 }
  };
  auto exchange_match_opt = arbitrage_strategy_matcher.FindMatch(ticker_map);
  EXPECT_EQ(true, exchange_match_opt.has_value());
  auto exchange_match = exchange_match_opt.value();
  EXPECT_EQ("B", exchange_match.m_best_bid.m_exchange);
  // TODO: fix calculation precision
  EXPECT_DOUBLE_EQ(0.0451, exchange_match.m_profit);
}