#include "http/exchange_client.h"
#include "http/binance_client.hpp"
#include "db/mongo_client.hpp"
#include "producer/mongo_ticker_producer.hpp"
#include "strategy/backtest_exchange_client.hpp"
#include "strategy/multi_arbitrage/arbitrage_strategy.hpp"
#include "strategy/ticker_broker.hpp"
#include "utils/config.hpp"

#include "json/json.hpp"

#include <cerrno>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
      std::cerr << "Please provide configuration file" << std::endl;
      return -1;
    }
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
    ArbitrageStrategyOptions strategy_opts;
    // // TODO: set options

    ArbitrageStrategy arbitrage_strategy(strategy_opts);
    BacktestSettings binance_backtest_settings;
    binance_backtest_settings.m_slippage = 5;
    binance_backtest_settings.m_fee = 0.0025;
    BacktestExchangeClient* binance_backtest_client = new BacktestExchangeClient(binance_backtest_settings, "binance_backtest_results.csv");
    BacktestSettings kraken_backtest_settings;
    kraken_backtest_settings.m_slippage = 5;
    kraken_backtest_settings.m_fee = 0.0025;
    BacktestExchangeClient* kraken_backtest_client = new BacktestExchangeClient(kraken_backtest_settings, "kraken_backtest_results.csv");
    arbitrage_strategy.RegisterExchangeClient("binance", binance_backtest_client);
    arbitrage_strategy.RegisterExchangeClient("kraken", kraken_backtest_client);
    TickerBroker ticker_broker({&arbitrage_strategy, binance_backtest_client, kraken_backtest_client});
    MongoTickerProducer mongo_producer(mongo_client, "findata", "BtcUsdTicker_v4", &ticker_broker);
    int64_t count = mongo_producer.Produce();
    std::cout << "Produced " + std::to_string(count) + " tickers" << std::endl;
    //arbitrage_strategy.PrintStats();
    return 0;
}
