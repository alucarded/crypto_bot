#include "exchange/exchange_client.h"
#include "http/binance_client.hpp"
#include "db/mongo_client.hpp"
#include "producer/mongo_ticker_producer.hpp"
#include "strategy/backtest_exchange_client.hpp"
#include "strategy/multi_arbitrage/arbitrage_strategy.hpp"
#include "utils/config.hpp"

#include "json/json.hpp"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include <cerrno>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

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

  BacktestSettings binance_backtest_settings;
  // TODO: FIXME: slippage value per pair
  binance_backtest_settings.m_exchange = "binance";
  binance_backtest_settings.slippage = 0;
  binance_backtest_settings.fee = 0.00075;
  BacktestExchangeClient* binance_backtest_client = new BacktestExchangeClient(binance_backtest_settings, "binance_backtest_results.csv");
  BacktestSettings kraken_backtest_settings;
  kraken_backtest_settings.m_exchange = "kraken";
  kraken_backtest_settings.slippage = 0;
  kraken_backtest_settings.fee = 0.0024;
  BacktestExchangeClient* kraken_backtest_client = new BacktestExchangeClient(kraken_backtest_settings, "kraken_backtest_results.csv");

  AccountManager* binance_account_manager = new AccountManager(binance_backtest_client);
  AccountManager* kraken_account_manager = new AccountManager(kraken_backtest_client);

  ArbitrageStrategyOptions strategy_opts;
  ExchangeParams binance_params;
  binance_params.slippage = 0.0;
  binance_params.fee = 0.00075;
  binance_params.daily_volume = 10.0;
  ExchangeParams kraken_params;
  kraken_params.slippage = 0.0;
  kraken_params.fee = 0.0026;
  kraken_params.daily_volume = 2.0;
  strategy_opts.exchange_params = {
    { "binance", binance_params },
    { "kraken", kraken_params }
  };
  strategy_opts.default_amount = {
    {SymbolId::ADA, 50},
    {SymbolId::BTC, 0.001},
    {SymbolId::DOT, 1.5},
    {SymbolId::ETH, 0.03},
    {SymbolId::EOS, 12},
    {SymbolId::XLM, 100},
  };
  strategy_opts.min_amount = {
    {SymbolId::ADA, 25},
    {SymbolId::BTC, 0.0002},
    {SymbolId::DOT, 0.5},
    {SymbolId::ETH, 0.005},
    {SymbolId::EOS, 2.5},
    {SymbolId::XLM, 20},
  };
  strategy_opts.max_ticker_age_us = 1000000; // 1s
  strategy_opts.max_ticker_delay_us = 1000000; // 1s
  strategy_opts.min_trade_interval_us = 0;
  strategy_opts.arbitrage_match_profit_margin = 0;
  strategy_opts.time_provider_fcn = [](const Ticker& ticker) { return ticker.m_arrived_ts; };
  ArbitrageStrategy arbitrage_strategy(strategy_opts);
  arbitrage_strategy.RegisterExchangeClient("binance", binance_account_manager);
  arbitrage_strategy.RegisterExchangeClient("kraken", kraken_account_manager);
  arbitrage_strategy.Initialize();

  MongoTickerProducer mongo_producer(mongo_client, config_json["db"].get<std::string>(), config_json["collection"].get<std::string>());
  // First register clients, so that execution price is same as seen price
  mongo_producer.Register(binance_backtest_client);
  mongo_producer.Register(kraken_backtest_client);
  mongo_producer.Register(&arbitrage_strategy);

  int64_t count = mongo_producer.Produce();
  std::cout << "Produced " + std::to_string(count) + " tickers" << std::endl;
  //arbitrage_strategy.PrintStats();
  return 0;
}
