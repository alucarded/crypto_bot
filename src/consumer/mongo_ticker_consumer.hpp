#include "db/mongo_client.hpp"
#include "ticker.h"
#include "websocket/ticker_client.hpp"
#include "consumer/consumer.h"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/instance.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

class MongoTickerConsumer : public Consumer<RawTicker> {
public:
    MongoTickerConsumer(MongoClient* mongo_client, const std::string& db_name, const std::string& coll_name)
            : m_mongo_client(mongo_client), m_db_name(db_name), m_coll_name(coll_name) {
        // Create index
        mongocxx::pool::entry client_entry = m_mongo_client->Get();
        mongocxx::client& client = *client_entry;
        auto db = client[m_db_name];
        auto coll = db[m_coll_name];
        document index_builder;
        mongocxx::options::index index_options{};
        index_options.unique(true);
        index_builder << "exchange" << 1 
            << "minute_utc" << 1;
        coll.create_index(index_builder.view(), index_options);
    }

    // This can be called from multiple threads
    virtual void Consume(const RawTicker& ticker) override {
        mongocxx::pool::entry client_entry = m_mongo_client->Get();
        mongocxx::client& client = *client_entry;
        auto db = client[m_db_name];
        auto coll = db[m_coll_name];

        using namespace std::chrono;
        // Make sure system clock is adjusted in controlled manner
        // on the machine where this runs.
        auto now = system_clock::now();
        system_clock::duration tp = now.time_since_epoch();
        minutes mins = duration_cast<minutes>(tp);
        microseconds us = duration_cast<microseconds>(tp);
        // Save in bucket related to arrival minute (now)
        // Then during backtesting check what was the delay
        // and its impact on order execution.
        coll.update_one(
            document{}
                << "exchange" << ticker.m_exchange
                << "minute_utc" << mins.count()
                << finalize,
            document{}
                << "$push" << bsoncxx::builder::stream::open_document << "tickers"
                << bsoncxx::builder::stream::open_document
                << "bid" << ticker.m_bid
                << "bid_vol" << ticker.m_bid_vol
                << "ask" << ticker.m_ask
                << "ask_vol" << ticker.m_ask_vol
                // Arrived timestamp in microseconds
                << "a_us" << us.count()
                // Source timestamp in microseconds
                << "s_us" << ticker.m_source_ts
                << bsoncxx::builder::stream::close_document
                << bsoncxx::builder::stream::close_document
                << finalize,
            mongocxx::options::update().upsert(true));
    }
private:
    MongoClient* m_mongo_client;
    std::string m_db_name;
    std::string m_coll_name;
};