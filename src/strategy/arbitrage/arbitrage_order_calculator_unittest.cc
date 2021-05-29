#include "arbitrage_order_calculator.h"

#include "gtest/gtest.h"

using namespace testing;

TEST(ArbitrageOrderCalculatorTest, CalculateTest) {
  ArbitrageStrategyOptions strategy_opts;
  strategy_opts.exchange_params = {
    { "binance", ExchangeParams("binance", 0.0, 0.00075, 52.506572519) },
    { "kraken", ExchangeParams("kraken", 0.0, 0.0026, 3.404118345) }
  };
  strategy_opts.default_amount = {
    {SymbolId::ADA, 50},
    {SymbolId::BTC, 0.001},
    {SymbolId::DOT, 1.5},
    {SymbolId::ETH, 0.03},
    {SymbolId::EOS, 12},
    {SymbolId::XLM, 100},
  };
  ArbitrageOrderCalculator arbitrage_order_calculator(strategy_opts);
  Ticker best_bid_ticker;
  best_bid_ticker.m_exchange = "binance";
  best_bid_ticker.m_bid = 1.7762;
  best_bid_ticker.m_bid_vol = 30000;
  best_bid_ticker.m_symbol = SymbolPairId::ADA_USDT;
  Ticker best_ask_ticker;
  best_ask_ticker.m_exchange = "kraken";
  best_ask_ticker.m_ask = 1.7674;
  best_ask_ticker.m_ask_vol = 20000;
  best_ask_ticker.m_symbol = SymbolPairId::ADA_USDT;
  ArbitrageOrders orders = arbitrage_order_calculator.Calculate(best_bid_ticker, best_ask_ticker, 100000, 1000000);
  // Prices
  double sell_price = orders.sell_order.GetPrice();
  double buy_price = orders.buy_order.GetPrice();
  std::cerr << sell_price << std::endl;
  std::cerr << buy_price << std::endl;
  EXPECT_TRUE(sell_price > buy_price);
  double profit = (1.0 - strategy_opts.exchange_params.at("binance").fee) * sell_price - (1.0 + strategy_opts.exchange_params.at("kraken").fee) * buy_price;
  // TODO: figure out precision, use EXPECT_DOUBLE_EQ
  EXPECT_NEAR(0, profit, 0.000000000000001);
  // TODO: more expectations
}