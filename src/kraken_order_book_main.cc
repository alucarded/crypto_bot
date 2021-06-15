
#include "exchange/dummy_exchange_listener.hpp"
#include "websocket/kraken_websocket_client.hpp"

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

  DummyExchangeListener exchange_listener;
  KrakenWebsocketClient kraken_websocket_client(&exchange_listener, true);

  std::promise<void> kraken_promise;
  std::future<void> kraken_future = kraken_promise.get_future();
  kraken_websocket_client.start(std::move(kraken_promise));
  kraken_future.wait();

  kraken_websocket_client.SubscribeOrderBook("ADA/USDT", KrakenOrderBookDepth::DEPTH_10);

  std::this_thread::sleep_until(std::chrono::time_point<std::chrono::system_clock>::max());
}
