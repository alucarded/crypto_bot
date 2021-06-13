#include "model/symbol.h"
#include "strategy/market_making/market_making_strategy.h"
#include "websocket/binance_trade_ticker_stream.hpp"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include <chrono>
#include <future>
#include <iostream>
#include <thread>

namespace logging = boost::log;
namespace keywords = boost::log::keywords;

void InitLogging() {
  // TODO: FIXME: why is it crashing?
  //logging::add_file_log(keywords::file_name = "cryptobot.log", boost::log::keywords::target = "/mnt/e/logs");
  //logging::add_console_log(std::cout, boost::log::keywords::format = ">> %Message%");
  logging::core::get()->set_filter(
      logging::trivial::severity >= logging::trivial::trace);
  logging::add_common_attributes();
}

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  InitLogging();
  BOOST_LOG_TRIVIAL(info) << "Boost logging configured";

  MarketMakingStrategy market_making_strategy;

  BinanceTradeTickerStream trade_ticker_stream(&market_making_strategy);
  std::promise<void> binance_promise;
  std::future<void> binance_future = binance_promise.get_future();
  trade_ticker_stream.start(std::move(binance_promise));
  binance_future.wait();

  trade_ticker_stream.SubscribeTicker("adausdt");

  std::this_thread::sleep_until(std::chrono::time_point<std::chrono::system_clock>::max());
};