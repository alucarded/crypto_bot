#include "account_manager_impl.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace testing;

class MockExchangeClient : public ExchangeClient {
public:
  MOCK_METHOD(std::string, GetExchange, (), (override));
  MOCK_METHOD(Result<Order>, MarketOrder, (SymbolPairId symbol, Side side, double qty), (override));
  MOCK_METHOD(Result<Order>, LimitOrder, (SymbolPairId symbol, Side side, double qty, double price), (override));
  MOCK_METHOD(Result<AccountBalance>, GetAccountBalance, (), (override));
  MOCK_METHOD(Result<std::vector<Order>>, GetOpenOrders, (), (override));
  MOCK_METHOD(void, CancelAllOrders, (), (override));
};

TEST(AccountManagerTest, OnOrderUpdateTest) { 
  std::unique_ptr<MockExchangeClient> mec{new MockExchangeClient()};
  AccountManagerImpl account_manager{mec.get()};

  Order new_order = Order::CreateBuilder()
      .Id("TEST_ID")
      .ClientId("")
      .Symbol(SymbolPairId::BTC_USDT)
      .Side_(Side::BUY)
      .OrderType_(OrderType::MARKET)
      .Quantity(0.001)
      .Price(50000)
      .OrderStatus_(OrderStatus::NEW)
      .Build();
  EXPECT_CALL(*mec.get(), MarketOrder(_, _, _))
    .Times(1)
    // Here we return order with price, but usually the initial response will not contain it.
    .WillOnce(Return(Result<Order>("", new_order)));
  account_manager.MarketOrder(SymbolPairId::BTC_USDT, Side::BUY, 0.001);

  Order closed_order = Order::CreateBuilder()
      .Id("TEST_ID")
      .OrderStatus_(OrderStatus::FILLED)
      .Build();
  account_manager.OnOrderUpdate(closed_order);

  double btc_balance = account_manager.GetTotalBalance(SymbolId::BTC);
  EXPECT_NEAR(0.001, btc_balance, std::numeric_limits<double>::min());
  double usdt_balance = account_manager.GetTotalBalance(SymbolId::USDT);
  EXPECT_NEAR(-50.0, usdt_balance, std::numeric_limits<double>::min());
}

// TODO: more tests, this is very stateful and thus pretty complex class