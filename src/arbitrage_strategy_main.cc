#include "client/binance_client.hpp"
#include "client/kraken_client.hpp"
#include "strategy/multi_arbitrage/arbitrage_strategy.hpp"
#include "strategy/ticker_broker.hpp"
#include "websocket/binance_ticker_client.hpp"
#include "websocket/kraken_ticker_client.hpp"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>

#include <iostream>
#include <thread>

namespace logging = boost::log;

void InitLogging() {
  logging::add_file_log("/var/log/cryptobot/cryptobot.log");
  
  logging::core::get()->set_filter(
      logging::trivial::severity >= logging::trivial::info);
}

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  InitLogging();

  try {
    ArbitrageStrategyOptions strategy_opts;
    strategy_opts.m_exchange_params = {
      { "binance", ExchangeParams("binance", 10.0, 0.00075) },
      { "kraken", ExchangeParams("kraken", 10.0, 0.002) }
    };
    ArbitrageStrategy arbitrage_strategy(strategy_opts);
    BinanceClient binance_client;
    KrakenClient kraken_client;
    arbitrage_strategy.RegisterExchangeClient("binance", &binance_client);
    arbitrage_strategy.RegisterExchangeClient("kraken", &kraken_client);
    TickerBroker ticker_broker({&arbitrage_strategy});
    BinanceTickerClient binance_ticker_client(&ticker_broker);
    KrakenTickerClient kraken_ticker_client(&ticker_broker);
    binance_ticker_client.start();
    kraken_ticker_client.start();
    std::this_thread::sleep_until(std::chrono::time_point<std::chrono::system_clock>::max());
  } catch (websocketpp::exception const & e) {
      std::cout << e.what() << std::endl;
  } catch (std::exception const & e) {
      std::cout << e.what() << std::endl;
  } catch (...) {
      std::cout << "other exception" << std::endl;
  }
}
