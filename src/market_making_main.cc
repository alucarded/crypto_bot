#include "http/binance_client.hpp"
#include "model/symbol.h"
#include "strategy/market_making/market_making_strategy.h"
#include "websocket/binance_book_ticker_stream.hpp"
#include "websocket/binance_order_book_stream.hpp"
#include "websocket/binance_trade_ticker_stream.hpp"
#include "websocket/binance_user_data_stream.hpp"

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
      logging::trivial::severity >= logging::trivial::info);
  logging::add_common_attributes();
}

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  InitLogging();
  BOOST_LOG_TRIVIAL(info) << "Boost logging configured";

  // Risk manager, manages orders
  MarketMakingRiskMangerOptions risk_manager_options;
  risk_manager_options.default_order_qty = 100;
  risk_manager_options.exchange_fee = 0.00075;
  risk_manager_options.our_fee = 0.00025;
  BinanceClient binance_orders_client;
  MarketMakingRiskManager risk_manager(risk_manager_options, &binance_orders_client);

  // User data stream
  BinanceUserDataStream binance_stream = BinanceUserDataStream::Create(risk_manager);
  std::promise<void> binance_stream_promise;
  std::future<void> binance_user_future = binance_stream_promise.get_future();
  binance_stream.start(std::move(binance_stream_promise));
  binance_user_future.wait();

  MarketMakingStrategy market_making_strategy(risk_manager);

  // Market data streams
  BinanceBookTickerStream binance_book_ticker_stream(&market_making_strategy);
  std::promise<void> binance_book_ticker_promise;
  std::future<void> binance_book_ticker_future = binance_book_ticker_promise.get_future();
  binance_book_ticker_stream.start(std::move(binance_book_ticker_promise));
  binance_book_ticker_future.wait();

  binance_book_ticker_stream.SubscribeTicker("adausdt");

  BinanceTradeTickerStream trade_ticker_stream(&market_making_strategy);
  std::promise<void> binance_trade_ticker_promise;
  std::future<void> binance_trade_ticker_future = binance_trade_ticker_promise.get_future();
  trade_ticker_stream.start(std::move(binance_trade_ticker_promise));
  binance_trade_ticker_future.wait();

  trade_ticker_stream.SubscribeTicker("adausdt");

  BinanceClient binance_client;
  BinanceSettings binance_settings = binance_client.GetBinanceSettings();
  // TODO: specify pair from config
  BinanceOrderBookStream binance_order_book_stream(binance_settings.GetPairSettings(SymbolPairId::ADA_USDT), &binance_client, &market_making_strategy);
  std::promise<void> binance_order_book_promise;
  std::future<void> binance_order_book_future = binance_order_book_promise.get_future();
  binance_order_book_stream.start(std::move(binance_order_book_promise));
  
  std::this_thread::sleep_until(std::chrono::time_point<std::chrono::system_clock>::max());
};