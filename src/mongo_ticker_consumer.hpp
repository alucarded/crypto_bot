#include "ticker.h"
#include "ticker_client.hpp"
#include "ticker_consumer.h"

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

class MongoTickerConsumer : public TickerConsumer {
public:
    MongoTickerConsumer(const std::string& uri_str,
      const std::string& db_name,
      const std::string& coll_name) : m_instance(), m_uri(uri_str), m_client(m_uri) {
        m_db = m_client[db_name];
        m_coll = m_db[coll_name];

        // TODO: ensure compound index on exchange name and minute
    }

    virtual void Consume(const RawTicker& ticker) override {
        // TODO: FIXME: this method can be called from multiple threads - add Mongo client pooling
        using namespace std::chrono;
        auto now = system_clock::now();
        system_clock::duration tp = now.time_since_epoch();
        minutes mins = duration_cast<minutes>(tp);
        microseconds us = duration_cast<microseconds>(tp);
        // Save in bucket related to arrival minute (now)
        // Then during backtesting check what was the delay
        // and its impact on order execution.
        m_coll.update_one(
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
        //std::cout << "Got ticker" << ticker;
        // auto builder = bsoncxx::builder::stream::document{};
        // bsoncxx::document::value doc_value = builder
        //   << "exchange" << ticker_client.GetExchangeName()
        //   << "bucket" << "database"
        //   << "count" << 1
        //   << "versions" << bsoncxx::builder::stream::open_array
        //     << bsoncxx::builder::stream::open_document << "bid" << ticker.m_bid << bsoncxx::builder::stream::close_document
        //   << close_array
        //   << bsoncxx::builder::stream::finalize;
        // m_coll.insert_one(doc_value);
    }
private:
    mongocxx::instance m_instance;
    mongocxx::uri m_uri;
    mongocxx::client m_client;
    mongocxx::database m_db;
    mongocxx::collection m_coll;
};