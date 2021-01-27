#include "client/exchange_client.h"
#include "db/mongo_client.hpp"
#include "producer/mongo_ticker_producer.hpp"
#include "strategy/backtest_exchange_account.hpp"
#include "strategy/arbitrage_strategy.hpp"
#include "strategy/strategy_ticker_consumer.hpp"

#include "json/json.hpp"

#include <cerrno>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

using json = nlohmann::json;

json GetConfigJson(const std::string& config_path) {
  std::ifstream config_fs(config_path, std::ios::in | std::ios::binary);
  if (config_fs)
  {
    std::ostringstream contents;
    contents << config_fs.rdbuf();
    config_fs.close();
    return json::parse(contents.str());
  }
  throw(errno);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
      std::cerr << "Please provide configuration file" << std::endl;
      return -1;
    }
    std::string config_path(argv[1]);
    auto config_json = GetConfigJson(config_path);
    // TODO: take credentials from parameter store
    std::string pass("DRt99xd4o7PMfygqotE8");
    MongoClient* mongo_client =
            MongoClient::GetInstance()->CreatePool("mongodb://" + config_json["user"].get<std::string>() + ":" + pass
            + "@" + config_json["host"].get<std::string>() + ":" + config_json["port"].get<std::string>()
            + "/?authSource=" + config_json["authSource"].get<std::string>());
    std::cout << "Created Mongo client pool" << std::endl;
    pass.clear();
    ArbitrageStrategyOptions strategy_opts;
    // // TODO: set options
    // BacktestSettings backtest_settings;
    // backtest_settings.m_slippage = 5;
    // backtest_settings.m_fee = 0.0025;
    // BacktestExchangeAccount exchange_account(backtest_settings);
    std::map<std::string, ExchangeClient*> exchange_clients;
    // TODO: add clients
    ArbitrageStrategy arbitrage_strategy(strategy_opts, exchange_clients);
    StrategyTickerConsumer ticker_consumer(&arbitrage_strategy);
    MongoTickerProducer mongo_producer(mongo_client, "findata", "BtcUsdTicker_v4", &ticker_consumer);
    int64_t count = mongo_producer.Produce();
    std::cout << "Produced " + std::to_string(count) + " tickers" << std::endl;
    //arbitrage_strategy.PrintStats();
    return 0;
}
