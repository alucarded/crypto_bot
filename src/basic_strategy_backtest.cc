#include "db/mongo_client.hpp"
#include "db/mongo_ticker_producer.hpp"
#include "backtest/backtest_exchange_client.hpp"
#include "strategy/basic_strategy.hpp"

#include <iostream>

int main(int argc, char* argv[]) {
    // (void)argc;
    // (void)argv;
    int64_t last_mins_count = 5000;
    if (argc > 1) {
        last_mins_count = std::stoi(std::string(argv[1]));
    }
    std::cout << "Hello" << std::endl;
    // TODO: take credentials from parameter store ?
    MongoClient* mongo_client =
            MongoClient::GetInstance()->CreatePool("mongodb://app:DRt99xd4o7PMfygqotE8@3.10.107.166:28888/?authSource=findata");
    std::cout << "Created Mongo client pool" << std::endl;
    BasicStrategyOptions strategy_opts;
    strategy_opts.m_trading_exchange = "binance";
    strategy_opts.m_required_exchanges = 5;
    BacktestSettings backtest_settings;
    backtest_settings.slippage = 5;
    backtest_settings.fee = 0.0025;
    BacktestExchangeClient exchange_client(backtest_settings, "backtest_results.csv");
    BasicStrategy basic_strategy(strategy_opts, &exchange_client);
    MongoTickerProducer mongo_producer(mongo_client, "findata", "BtcUsdTicker_v4", &basic_strategy);
    mongo_producer.Produce(last_mins_count);
    basic_strategy.PrintStats();
}
