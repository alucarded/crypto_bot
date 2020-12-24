#include "producer/mongo_ticker_producer.hpp"
#include "db/mongo_client.hpp"

#include <iostream>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    // TODO: take credentials from parameter store ?
    MongoClient* mongo_client =
            MongoClient::GetInstance()->CreatePool("mongodb://app:DRt99xd4o7PMfygqotE8@3.10.107.166:28888/?authSource=findata");
    MongoTickerProducer mongo_producer(mongo_client, "findata", "BtcUsdTicker_v3", nullptr);
    mongo_producer.Produce(0, 100000000);
}
