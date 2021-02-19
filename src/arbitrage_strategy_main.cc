#include "http/binance_client.hpp"
#include "http/kraken_client.hpp"
#include "strategy/multi_arbitrage/arbitrage_strategy.hpp"
#include "strategy/ticker_broker.hpp"
#include "websocket/binance_websocket_client.hpp"
#include "websocket/kraken_websocket_client.hpp"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include <iostream>
#include <thread>

namespace logging = boost::log;
namespace keywords = boost::log::keywords;

void InitLogging() {
  // TODO: FIXME: why is it crashing?
  //logging::add_file_log(keywords::file_name = "cryptobot.log", boost::log::keywords::target = "/mnt/e/logs");
  //logging::add_console_log(std::cout, boost::log::keywords::format = ">> %Message%");
  logging::core::get()->set_filter(
      logging::trivial::severity >= logging::trivial::debug);
  logging::add_common_attributes();
}

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  InitLogging();
  BOOST_LOG_TRIVIAL(info) << "Boost logging configured";
  try {
    ArbitrageStrategyOptions strategy_opts;
    strategy_opts.m_exchange_params = {
      { "binance", ExchangeParams("binance", 5.0, 0.00075) },
      { "kraken", ExchangeParams("kraken", 5.0, 0.0026) }
    };
    strategy_opts.m_default_amount = 0.0002;
    strategy_opts.m_min_amount = 0.0002;
    strategy_opts.m_max_ticker_age_us = 1000000; // 1s
    strategy_opts.m_max_ticker_delay_us = 500000; // 500 ms
    strategy_opts.m_min_trade_interval_us = 500000; // 500 ms
    ArbitrageStrategy arbitrage_strategy(strategy_opts);
    BinanceClient binance_client;
    KrakenClient kraken_client;
    arbitrage_strategy.RegisterExchangeClient("binance", &binance_client);
    arbitrage_strategy.RegisterExchangeClient("kraken", &kraken_client);
    arbitrage_strategy.Initialize();
    TickerBroker ticker_broker({&arbitrage_strategy});
    BinanceWebsocketClient binance_websocket_client(&ticker_broker);
    KrakenWebsocketClient kraken_websocket_client(&ticker_broker);
    binance_websocket_client.start();
    kraken_websocket_client.start();
    std::this_thread::sleep_until(std::chrono::time_point<std::chrono::system_clock>::max());
  } catch (websocketpp::exception const & e) {
      std::cout << e.what() << std::endl;
  } catch (std::exception const & e) {
      std::cout << e.what() << std::endl;
  } catch (...) {
      std::cout << "other exception" << std::endl;
  }
}
