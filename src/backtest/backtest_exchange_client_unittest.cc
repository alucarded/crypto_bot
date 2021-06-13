#include "backtest_exchange_client.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace testing;
using ::testing::_;

class MockAccountBalanceListener : public AccountBalanceListener {
public:
  virtual ~MockAccountBalanceListener() {

  }

  MOCK_METHOD(void, OnAccountBalanceUpdate, (const AccountBalance& balance), (override));
};

TEST(BacktestExchangeClientTest, TestSingleMarketOrder) {
  BacktestSettings settings;
  settings.exchange = "test";
  settings.fee = 0.001;
  settings.slippage = 0.0;
  settings.network_latency_us = 100000;
  settings.execution_delay_us = 20000;
  settings.initial_balances = {
    {SymbolId::ADA, 100000.0},
    {SymbolId::USDT, 100000}
  };
  MockAccountBalanceListener balance_listener;
  BacktestExchangeClient backtest_client(settings, balance_listener);

  Ticker ticker1;
  ticker1.ask = 1.11;
  ticker1.ask_vol = 1000;
  ticker1.bid = 1.10;
  ticker1.bid_vol = 2000;
  ticker1.arrived_ts = 1;
  ticker1.exchange = "test";
  ticker1.symbol = SymbolPairId::ADA_USDT;
  backtest_client.OnBookTicker(ticker1);

  backtest_client.MarketOrder(SymbolPairId::ADA_USDT, Side::BUY, 1000);

  Ticker ticker2;
  ticker2.ask = 1.12;
  ticker2.ask_vol = 2000;
  ticker2.bid = 1.11;
  ticker2.bid_vol = 2000;
  ticker2.arrived_ts = 80000;
  ticker2.exchange = "test";
  ticker2.symbol = SymbolPairId::ADA_USDT;
  backtest_client.OnBookTicker(ticker2);

  AccountBalance ab;
  EXPECT_CALL(balance_listener, OnAccountBalanceUpdate(_))
    .WillOnce(testing::SaveArg<0>(&ab));

  Ticker ticker3;
  ticker3.ask = 1.13;
  ticker3.ask_vol = 4000;
  ticker3.bid = 1.11;
  ticker3.bid_vol = 1000;
  ticker3.arrived_ts = 230000;
  ticker3.exchange = "test";
  ticker3.symbol = SymbolPairId::ADA_USDT;
  backtest_client.OnBookTicker(ticker3);

  EXPECT_DOUBLE_EQ(98878.88, ab.GetFreeBalance(SymbolId::USDT));
}

TEST(BacktestExchangeClientTest, TestSingleLimitOrder) {
  BacktestSettings settings;
  settings.exchange = "test";
  settings.fee = 0.001;
  settings.slippage = 0.0;
  settings.network_latency_us = 100000;
  settings.execution_delay_us = 20000;
  settings.initial_balances = {
    {SymbolId::ADA, 100000.0},
    {SymbolId::USDT, 100000}
  };
  MockAccountBalanceListener balance_listener;
  BacktestExchangeClient backtest_client(settings, balance_listener);

  Ticker ticker1;
  ticker1.ask = 1.11;
  ticker1.ask_vol = 1000;
  ticker1.bid = 1.10;
  ticker1.bid_vol = 2000;
  ticker1.arrived_ts = 1;
  ticker1.exchange = "test";
  ticker1.symbol = SymbolPairId::ADA_USDT;
  backtest_client.OnBookTicker(ticker1);

  backtest_client.LimitOrder(SymbolPairId::ADA_USDT, Side::BUY, 1000, 1.09);

  Ticker ticker2;
  ticker2.ask = 1.12;
  ticker2.ask_vol = 2000;
  ticker2.bid = 1.11;
  ticker2.bid_vol = 2000;
  ticker2.arrived_ts = 230000;
  ticker2.exchange = "test";
  ticker2.symbol = SymbolPairId::ADA_USDT;
  backtest_client.OnBookTicker(ticker2);

  AccountBalance ab;
  EXPECT_CALL(balance_listener, OnAccountBalanceUpdate(_))
    .WillOnce(testing::SaveArg<0>(&ab));

  Ticker ticker3;
  ticker3.ask = 1.095;
  ticker3.ask_vol = 2000;
  ticker3.bid = 1.085;
  ticker3.bid_vol = 3000;
  ticker3.arrived_ts = 300000;
  ticker3.exchange = "test";
  ticker3.symbol = SymbolPairId::ADA_USDT;
  backtest_client.OnBookTicker(ticker3);

  EXPECT_DOUBLE_EQ(98903.905, ab.GetFreeBalance(SymbolId::USDT));
}

TEST(BacktestExchangeClientTest, TestSingleLimitOrderAsMarket) {
  
}