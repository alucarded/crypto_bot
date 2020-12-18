#include "ticker.h"
#include "ticker_client.hpp"

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

class MongoTickerCollector {
public:
    MongoTickerCollector(const std::string& uri_str,
      const std::string& db_name,
      const std::string& coll_name) : m_instance(), m_uri(uri_str), m_client(m_uri) {
        m_db = m_client[db_name];
        m_coll = m_db[coll_name];
    }

    void Collect(const TickerClient& ticker_client) {
        Ticker ticker = ticker_client.GetTicker();
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