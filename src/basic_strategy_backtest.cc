#include "db/mongo_client.hpp"
#include "producer/mongo_ticker_producer.hpp"
#include "strategy/backtest_exchange_account.hpp"
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
    backtest_settings.m_slippage = 5;
    backtest_settings.m_fee = 0.0025;
    BacktestExchangeAccount exchange_account(backtest_settings);
    BasicStrategy basic_strategy(strategy_opts, &exchange_account);
    MongoTickerProducer mongo_producer(mongo_client, "findata", "BtcUsdTicker_v4", &basic_strategy);
    mongo_producer.Produce(last_mins_count);
    basic_strategy.PrintStats();
}
