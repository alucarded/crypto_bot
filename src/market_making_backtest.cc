#include "backtest/backtest_exchange_client.hpp"
#include "backtest/backtest_results_processor.h"
#include "exchange/exchange_client.h"
#include "db/mongo_client.hpp"
#include "db/mongo_ticker_producer.hpp"
#include "strategy/market_making/market_making_strategy.h"
#include "utils/config.hpp"
#include "utils/string.h"

#include "json/json.hpp"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include <cerrno>
#include <iostream>

namespace logging = boost::log;
namespace keywords = boost::log::keywords;

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Please provide configuration file" << std::endl;
    return -1;
  }

  logging::core::get()->set_filter(
      logging::trivial::severity >= logging::trivial::info);

  std::string config_path(argv[1]);
  auto config_json = cryptobot::GetConfigJson(config_path);
  // TODO: take credentials from parameter store
  std::string pass("DRt99xd4o7PMfygqotE8");
  MongoClient* mongo_client =
          MongoClient::GetInstance()->CreatePool("mongodb://" + config_json["user"].get<std::string>() + ":" + pass
          + "@" + config_json["host"].get<std::string>() + ":" + config_json["port"].get<std::string>()
          + "/?authSource=" + config_json["authSource"].get<std::string>());
  std::cout << "Created Mongo client pool" << std::endl;
  pass.clear();

  BacktestResultsProcessor backtest_results_processor("backtest_results.csv", {SymbolId::USDT, SymbolId::ADA});
  BacktestSettings binance_backtest_settings;
  // TODO: FIXME: slippage value per pair
  binance_backtest_settings.exchange = "binance";
  binance_backtest_settings.slippage = 0;
  binance_backtest_settings.fee = 0.00075;
  binance_backtest_settings.network_latency_us = 200000;
  binance_backtest_settings.execution_delay_us = 20000;
  binance_backtest_settings.initial_balances = {
    {SymbolId::ADA, 10000.0},
    {SymbolId::USDT, 24000}
  };
  BacktestExchangeClient binance_backtest_client(binance_backtest_settings, backtest_results_processor);

  // Risk manager, manages orders
  MarketMakingRiskMangerOptions risk_manager_options;
  risk_manager_options.default_order_qty = 100;
  risk_manager_options.exchange_fee = 0.00075;
  risk_manager_options.our_fee = 0.00025;
  MarketMakingRiskManager risk_manager(risk_manager_options, &binance_backtest_client);

  MarketMakingStrategy market_making_strategy(risk_manager);

  MongoTickerProducer mongo_producer(mongo_client, config_json["db"].get<std::string>(), config_json["collection"].get<std::string>());
  // First register clients, so that execution price is same as seen price
  // mongo_producer.Register(&binance_backtest_client);
  // mongo_producer.Register(&kraken_backtest_client);
  mongo_producer.Register(&market_making_strategy);

  int64_t count = mongo_producer.Produce();
  std::cout << "Produced " + std::to_string(count) + " events" << std::endl;

  return 0;
}
