#include "consumer/strategy_ticker_consumer.hpp"
#include "db/mongo_client.hpp"
#include "producer/mongo_ticker_producer.hpp"
#include "strategy/basic_strategy.hpp"

#include <iostream>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    std::cout << "Hello" << std::endl;
    // TODO: take credentials from parameter store ?
    MongoClient* mongo_client =
            MongoClient::GetInstance()->CreatePool("mongodb://app:DRt99xd4o7PMfygqotE8@3.10.107.166:28888/?authSource=findata");
    std::cout << "Created Mongo client pool" << std::endl;
    StrategyOptions strategy_opts;
    strategy_opts.m_trading_exchange = "binance";
    strategy_opts.m_required_exchanges = 8;
    BasicStrategy basic_strategy(strategy_opts);
    StrategyTickerConsumer strategy_ticker_consumer(&basic_strategy);
    MongoTickerProducer mongo_producer(mongo_client, "findata", "BtcUsdTicker_v3", &strategy_ticker_consumer);
    mongo_producer.Produce(1000);
}
