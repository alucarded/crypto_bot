#include "binapi/api.hpp"

#include <boost/asio/io_context.hpp>

#include "gtest/gtest.h"

#include <iostream>

TEST(BinanceClientTest, BasicTest)
{
  boost::asio::io_context ioctx;
  binapi::rest::api api{
      ioctx
      ,"api.binance.com"
      ,"443"
      ,"" // can be empty for non USER_DATA reqs
      ,"" // can be empty for non USER_DATA reqs
      ,10000 // recvWindow
  };

  auto res = api.price("BTCUSDT");
  if ( !res ) {
      std::cerr << "get price error: " << res.errmsg << std::endl;
  }

  std::cout << "price: " << res.v << std::endl;
}