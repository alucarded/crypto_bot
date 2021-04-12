#include "exchange/exchange_listener.h"
#include "http/binance_client.hpp"
#include "http/kraken_client.hpp"
#include "model/symbol.h"
#include "strategy/multi_arbitrage/arbitrage_strategy.hpp"
//#include "strategy/ticker_broker.hpp"
#include "websocket/binance_websocket_client.hpp"
#include "websocket/binance_user_data_stream.hpp"
#include "websocket/kraken_user_data_stream.hpp"
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
      logging::trivial::severity >= logging::trivial::debug);
  logging::add_common_attributes();
}

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  InitLogging();
  BOOST_LOG_TRIVIAL(info) << "Boost logging configured";

  ArbitrageStrategyOptions strategy_opts;
  strategy_opts.m_exchange_params = {
    { "binance", ExchangeParams("binance", 0.0, 0.00075) },
    { "kraken", ExchangeParams("kraken", 0.0, 0.0026) }
  };
  strategy_opts.m_default_amount = {
    {SymbolId::ADA, 50},
    {SymbolId::BTC, 0.001},
    {SymbolId::DOT, 1.5},
    {SymbolId::ETH, 0.03},
    {SymbolId::EOS, 12},
    {SymbolId::XLM, 100},
  };
  strategy_opts.m_min_amount = {
    {SymbolId::ADA, 25},
    {SymbolId::BTC, 0.0002},
    {SymbolId::DOT, 0.5},
    {SymbolId::ETH, 0.005},
    {SymbolId::EOS, 2.5},
    {SymbolId::XLM, 20},
  };
  strategy_opts.m_max_ticker_age_us = 1000000; // 1s
  strategy_opts.m_max_ticker_delay_us = 2000000; // 1s
  strategy_opts.m_min_trade_interval_us = 0;
  strategy_opts.m_base_currency_ratio = 0.5;
  strategy_opts.m_allowed_deviation = 0.3;

  BinanceClient* binance_client = new BinanceClient();
  AccountManager* binance_account_manager = new AccountManager(binance_client);
  KrakenClient* kraken_client = new KrakenClient();
  AccountManager* kraken_account_manager = new AccountManager(kraken_client);

  BinanceUserDataStream binance_stream = BinanceUserDataStream::Create(binance_client, binance_account_manager);
  std::promise<void> binance_stream_promise;
  std::future<void> binance_user_future = binance_stream_promise.get_future();
  binance_stream.start(std::move(binance_stream_promise));
  binance_user_future.wait();

  KrakenUserDataStream kraken_stream(kraken_client, kraken_account_manager);
  std::promise<void> kraken_stream_promise;
  std::future<void> kraken_user_future = kraken_stream_promise.get_future();
  kraken_stream.start(std::move(kraken_stream_promise));
  kraken_user_future.wait();

  ArbitrageStrategy arbitrage_strategy(strategy_opts);
  arbitrage_strategy.RegisterExchangeClient("binance", binance_account_manager);
  arbitrage_strategy.RegisterExchangeClient("kraken", kraken_account_manager);
  // Initialization is done when user data stream connection is opened
  // arbitrage_strategy.Initialize();

  BinanceWebsocketClient binance_websocket_client(&arbitrage_strategy);
  KrakenWebsocketClient kraken_websocket_client(&arbitrage_strategy);

  std::promise<void> binance_promise;
  std::future<void> binance_future = binance_promise.get_future();
  binance_websocket_client.start(std::move(binance_promise));
  std::promise<void> kraken_promise;
  std::future<void> kraken_future = kraken_promise.get_future();
  kraken_websocket_client.start(std::move(kraken_promise));
  binance_future.wait();
  kraken_future.wait();

  auto wait_time = 400ms;
  binance_websocket_client.SubscribeTicker("btcusdt");
  std::this_thread::sleep_for(wait_time);
  binance_websocket_client.SubscribeTicker("adausdt");
  std::this_thread::sleep_for(wait_time);
  binance_websocket_client.SubscribeTicker("dotusdt");
  std::this_thread::sleep_for(wait_time);
  binance_websocket_client.SubscribeTicker("eosusdt");
  std::this_thread::sleep_for(wait_time);
  binance_websocket_client.SubscribeTicker("ethusdt");
  std::this_thread::sleep_for(wait_time);
  binance_websocket_client.SubscribeTicker("adabtc");
  std::this_thread::sleep_for(wait_time);
  binance_websocket_client.SubscribeTicker("dotbtc");
  std::this_thread::sleep_for(wait_time);
  binance_websocket_client.SubscribeTicker("eosbtc");
  std::this_thread::sleep_for(wait_time);
  binance_websocket_client.SubscribeTicker("eoseth");
  std::this_thread::sleep_for(wait_time);
  binance_websocket_client.SubscribeTicker("ethbtc");
  std::this_thread::sleep_for(wait_time);
  binance_websocket_client.SubscribeTicker("xlmbtc");

  kraken_websocket_client.SubscribeOrderBook("XBT/USDT");
  std::this_thread::sleep_for(wait_time);
  kraken_websocket_client.SubscribeOrderBook("ADA/USDT");
  std::this_thread::sleep_for(wait_time);
  kraken_websocket_client.SubscribeOrderBook("DOT/USDT");
  std::this_thread::sleep_for(wait_time);
  kraken_websocket_client.SubscribeOrderBook("EOS/USDT");
  std::this_thread::sleep_for(wait_time);
  kraken_websocket_client.SubscribeOrderBook("ETH/USDT");
  std::this_thread::sleep_for(wait_time);
  kraken_websocket_client.SubscribeOrderBook("ADA/XBT");
  std::this_thread::sleep_for(wait_time);
  kraken_websocket_client.SubscribeOrderBook("DOT/XBT");
  std::this_thread::sleep_for(wait_time);
  kraken_websocket_client.SubscribeOrderBook("EOS/XBT");
  std::this_thread::sleep_for(wait_time);
  kraken_websocket_client.SubscribeOrderBook("EOS/ETH");
  std::this_thread::sleep_for(wait_time);
  kraken_websocket_client.SubscribeOrderBook("ETH/XBT");
  std::this_thread::sleep_for(wait_time);
  kraken_websocket_client.SubscribeOrderBook("XLM/XBT");

  std::this_thread::sleep_until(std::chrono::time_point<std::chrono::system_clock>::max());
}
