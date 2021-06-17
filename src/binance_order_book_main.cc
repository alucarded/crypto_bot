
#include "exchange/dummy_exchange_listener.hpp"
#include "exchange/symbol_pair_settings.hpp"
#include "http/binance_client.hpp"
#include "websocket/binance_order_book_stream.hpp"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

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

  BinanceClient binance_client;
  BinanceSettings binance_settings = binance_client.GetBinanceSettings();

  DummyExchangeListener exchange_listener;
  BinanceOrderBookStream binance_book_stream(binance_settings.GetPairSettings(SymbolPairId::ADA_USDT), &binance_client, &exchange_listener);

  std::promise<void> binance_promise;
  std::future<void> binance_future = binance_promise.get_future();
  binance_book_stream.start(std::move(binance_promise));
  binance_future.wait();

  std::this_thread::sleep_until(std::chrono::time_point<std::chrono::system_clock>::max());
}
