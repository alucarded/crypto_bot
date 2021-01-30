#include "arbitrage_strategy_matcher.hpp"

#include "gtest/gtest.h"

class ArbitrageStrategyMatcherFixture : public testing::Test { 
public:
  ArbitrageStrategyMatcherFixture() {
    // initialization code here
  }
  
  void SetUp() {
    // code here will execute just before the test ensues
  }
  
  void TearDown() {
    // code here will be called just after the test completes
    // ok to through exceptions from here if need be
  }
  
  ~ArbitrageStrategyMatcherFixture()  {
    // cleanup any pending stuff, but no exceptions allowed
  }

protected:
  // put in any custom data members that you need
  ArbitrageStrategyMatcher arbitrage_strategy_matcher;
};

TEST_F(ArbitrageStrategyMatcherFixture, OnTickerTest)
{
  Ticker ticker1;
  ticker1.m_ask = 1.1;
  ticker1.m_exchange = "A";
  Ticker ticker2;
  ticker2.m_bid = 1.2;
  ticker2.m_exchange = "B";
  std::map<std::string, Ticker> ticker_map = {
    { ticker1.m_exchange, ticker1 },
    { ticker2.m_exchange, ticker2 }
  };
  auto exchange_pair_opt = arbitrage_strategy_matcher.FindMatch(ticker_map);
  EXPECT_EQ(true, exchange_pair_opt.has_value());	
}