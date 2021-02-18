#include "binapi/api.hpp"

#include "utils/timer.hpp"

#include <boost/asio/io_context.hpp>

#include "gtest/gtest.h"

#include <iostream>

namespace {

// HMAC_SHA256, Good_guy_12
// Do not worry, these are testnet keys
const std::string g_api_key = "44pNgqvCmQcPAEjNY4I96TflqFRf4oQaQUbmkbxISdKGGmJojJ1kH8LkOYy6JIUD";
const std::string g_secret = "giavUBTzRVa91h58H8GhQ0UWqfGFW7Mu93PF5oXPG6AMKWIlbpbpQ6E9YTDycOAJ";

}

TEST(BinanceClientTest, NewOrderTest)
{
  boost::asio::io_context ioctx;
  binapi::rest::api api{
      ioctx
      ,"testnet.binance.vision"
      ,"443"
      ,g_api_key // can be empty for non USER_DATA reqs
      ,g_secret // can be empty for non USER_DATA reqs
      ,1000 // recvWindow
      , "cryptobot-1.0.0"
  };
  cryptobot::Timer timer;
  timer.start();
  api.new_order("BTCUSDT", binapi::e_side::buy, binapi::e_type::market, binapi::e_time::IOC,"0.0002",
        std::string(), "1", std::string(), std::string()
        // ,
        // [&] (const char *fl, int ec, std::string emsg, auto res) {
        //     timer.stop();
        //     std::cout << "Milliseconds: " << timer.elapsedMilliseconds() << std::endl;
        //     if ( ec ) {
        //         std::cerr << "New order error: fl=" << fl << ", ec=" << ec << ", emsg=" << emsg << std::endl;
        //         return false;
        //     }
        //     std::cout << "New order: " << res << std::endl;
        //     const auto& res_obj = res.get_response_full();
        //     EXPECT_STREQ("BTCUSDT", res_obj.symbol.c_str());
        //     return true;
        // }
    );
  //ioctx.run();
}

TEST(BinanceClientTest, AccountInfoTest)
{
  boost::asio::io_context ioctx;
  binapi::rest::api api{
      ioctx
      ,"testnet.binance.vision"
      ,"443"
      ,g_api_key // can be empty for non USER_DATA reqs
      ,g_secret // can be empty for non USER_DATA reqs
      ,1000 // recvWindow
      , "cryptobot-1.0.0"
  };
  cryptobot::Timer timer;
  timer.start();
  auto res = api.account_info();
  std::cout << res.v << std::endl;
}