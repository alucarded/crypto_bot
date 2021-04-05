#include "model/order_book.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace testing;

TEST(OrderBookTest, BidOrderTest) { 
  OrderBook ob{"test", SymbolPairId::BTC_USDT, PrecisionSettings{8, 8, 8}};
  PriceLevel pl1(price_t(473832), 0.234, 1);
  ob.UpsertBid(pl1);
  PriceLevel pl2(price_t(473821), 0.1, 2);
  ob.UpsertBid(pl2);
  PriceLevel pl3(price_t(473823), 0.2, 2);
  ob.UpsertBid(pl3);
  PriceLevel pl4(price_t(473821), 0.4, 2);
  ob.UpsertBid(pl4);
  ASSERT_THAT(ob.GetBids(), ElementsAre(Eq(pl4), Eq(pl3), Eq(pl1)));
  ob.DeleteBid(price_t(473832));
  ASSERT_THAT(ob.GetBids(), ElementsAre(Eq(pl4), Eq(pl3)));
  ob.DeleteBid(price_t(473821));
  ASSERT_THAT(ob.GetBids(), ElementsAre(Eq(pl3)));
  ob.DeleteBid(price_t(473823));
  ASSERT_TRUE(ob.GetBids().empty());
}

TEST(OrderBookTest, AskOrderTest) { 
  OrderBook ob{"test", SymbolPairId::BTC_USDT, PrecisionSettings{8, 8, 8}};
  PriceLevel pl1(price_t(473832), 0.234, 1);
  ob.UpsertAsk(pl1);
  PriceLevel pl2(price_t(473821), 0.1, 2);
  ob.UpsertAsk(pl2);
  PriceLevel pl3(price_t(473823), 0.2, 2);
  ob.UpsertAsk(pl3);
  PriceLevel pl4(price_t(473821), 0.4, 2);
  ob.UpsertAsk(pl4);
  ASSERT_THAT(ob.GetAsks(), ElementsAre(Eq(pl1), Eq(pl3), Eq(pl4)));
  ob.DeleteAsk(price_t(473832));
  ASSERT_THAT(ob.GetAsks(), ElementsAre(Eq(pl3), Eq(pl4)));
  ob.DeleteAsk(price_t(473823));
  ASSERT_THAT(ob.GetAsks(), ElementsAre(Eq(pl4)));
  ob.DeleteAsk(price_t(473821));
  ASSERT_TRUE(ob.GetAsks().empty());
}