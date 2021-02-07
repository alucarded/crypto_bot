#include "client/binance_client.hpp"
#include "client/kraken_client.hpp"
#include "strategy/multi_arbitrage/arbitrage_strategy.hpp"
#include "strategy/ticker_broker.hpp"
#include "websocket/binance_ticker_client.hpp"
#include "websocket/kraken_ticker_client.hpp"

#include <iostream>
#include <thread>

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  try {
    ArbitrageStrategyOptions strategy_opts;
  // // TODO: set options

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
