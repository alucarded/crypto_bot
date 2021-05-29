#include "arbitrage_strategy.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

class MockMatcher : public ArbitrageStrategyMatcher {
public:
  MOCK_METHOD1(FindMatch, std::optional<ArbitrageStrategyMatch>(const std::map<std::string, Ticker>&));
};

class ArbitrageStrategyFixture : public testing::Test { 
public:
  ArbitrageStrategyFixture() {
    ArbitrageStrategyOptions options;
    options.exchange_params = {
      {"A", ExchangeParams("A", 0.01, 0.01)},
      {"B", ExchangeParams("B", 0.01, 0.02)}
    };
    options.default_amount = 0.0005;
    MockMatcher mock_matcher;
    arbitrage_strategy = ArbitrageStrategy(options, mock_matcher);
  }

  virtual void SetUp() override {
    // no-op
  }

  virtual void TearDown() override {
    // no-op
  }

  ~ArbitrageStrategyFixture()  {
  }

protected:
  ArbitrageStrategy arbitrage_strategy;
};

TEST_F(ArbitrageStrategyFixture, OnTickerTest)
{
  // Ticker ticker1;
  // ticker1.m_ask = 1.1;
  // ticker1.m_bid = 1.0;
  // ticker1.m_exchange = "A";
  // Ticker ticker2;
  // ticker2.m_ask = 1.3;
  // ticker2.m_bid = 1.2;
  // ticker2.m_exchange = "B";
  // std::map<std::string, Ticker> ticker_map = {
  //   { ticker1.m_exchange, ticker1 },
  //   { ticker2.m_exchange, ticker2 }
  // };
  // auto exchange_match_opt = arbitrage_strategy_matcher.FindMatch(ticker_map);
  // EXPECT_EQ(true, exchange_match_opt.has_value());
  // auto exchange_match = exchange_match_opt.value();
  // EXPECT_EQ("B", exchange_match.m_best_bid.m_exchange);
  // EXPECT_NEAR(0.05, exchange_match.m_profit, 0.00001);
}