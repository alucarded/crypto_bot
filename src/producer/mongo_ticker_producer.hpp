#include "consumer/consumer.h"
#include "db/mongo_client.hpp"
#include "ticker.h"

#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/instance.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>

#include <cassert>
#include <limits>
#include <vector>

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

class MongoTickerProducer {
public:
  MongoTickerProducer(MongoClient* mongo_client, const std::string& db_name,
      const std::string &coll_name, Consumer<RawTicker>* ticker_consumer)
      : m_mongo_client(mongo_client), m_db_name(db_name), m_coll_name(coll_name), m_ticker_consumer(ticker_consumer) {

  }

  void Produce(const int64_t last_mins) {
    using namespace std::chrono;
    auto now = system_clock::now();
    system_clock::duration dur = now.time_since_epoch();
    minutes mins = duration_cast<minutes>(dur);
    Produce(mins.count() - last_mins, mins.count());
  }

  void Produce(const int64_t from_min, const int64_t to_min) {
    mongocxx::pool::entry client_entry = m_mongo_client->Get();
    mongocxx::client& client = *client_entry;
    auto db = client[m_db_name];
    auto coll = db[m_coll_name];
    // Get Mongo cursor
    mongocxx::cursor cursor = coll.find(document{} << "minute_utc"
        << open_document
        << "$gte" << from_min
        << "$lt" << to_min
        << close_document << finalize);
    Produce(cursor);
  }

  void Produce() {
    mongocxx::pool::entry client_entry = m_mongo_client->Get();
    mongocxx::client& client = *client_entry;
    auto db = client[m_db_name];
    auto coll = db[m_coll_name];
    // Get Mongo cursor
    mongocxx::cursor cursor = coll.find({});
    Produce(cursor);
  }

private:

  void Produce(mongocxx::cursor& cursor) {
    // Put all tickers in a vector
    std::vector<RawTicker> tickers_vec;
    int64_t prev_min_bucket = 0;
    for(auto doc : cursor) {
      // std::getchar();
      // std::cout << bsoncxx::to_json(doc) << "\n";
      bsoncxx::document::element minute_utc_elem = doc["minute_utc"];
      int64_t minute_utc = minute_utc_elem.get_int64().value;
      bsoncxx::document::element exchange_elem = doc["exchange"];
      bsoncxx::document::element tickers_elem = doc["tickers"];
      bsoncxx::types::b_array arr = tickers_elem.get_array();
      for (bsoncxx::array::element ticker_doc : arr.value) {
        RawTicker raw_ticker;
        raw_ticker.m_bid = ticker_doc["bid"].get_value().get_utf8().value.to_string();
        raw_ticker.m_bid_vol = ticker_doc["bid_vol"].get_value().get_utf8().value.to_string();
        raw_ticker.m_ask = ticker_doc["ask"].get_value().get_utf8().value.to_string();
        raw_ticker.m_ask_vol = ticker_doc["ask_vol"].get_value().get_utf8().value.to_string();
        raw_ticker.m_source_ts = ticker_doc["s_us"].get_int64().value;
        raw_ticker.m_arrived_ts = ticker_doc["a_us"].get_int64().value;
        raw_ticker.m_exchange = exchange_elem.get_value().get_utf8().value.to_string();
        raw_ticker.m_symbol = doc["symbol"] ? doc["symbol"].get_value().get_utf8().value.to_string() : "";
        tickers_vec.push_back(raw_ticker);
        // std::cout << "Added ticker:" << std::endl;
        // std::cout << raw_ticker;
      }

      if (tickers_vec.size() > 10000) {
        std::cout << "Flushing tickers..." << std::endl;
        std::sort(tickers_vec.begin(), tickers_vec.end(), [](const RawTicker& a, const RawTicker& b) -> bool {
          // Here we assume ticks always arrive in the right order
          // In strategy part we can verify it and compare source timestamps and arrived timestamps
          return a.m_arrived_ts < b.m_arrived_ts;
        });
        if (m_ticker_consumer) {
          std::for_each(tickers_vec.begin(), tickers_vec.end(), [&](const RawTicker& rt) {
            m_ticker_consumer->Consume(rt);
          });
        }
        tickers_vec.clear();
      }
      assert(minute_utc >= prev_min_bucket);
      prev_min_bucket = minute_utc;
    }
  }

  MongoClient* m_mongo_client;
  const std::string m_db_name;
  const std::string m_coll_name;
  Consumer<RawTicker>* m_ticker_consumer;
};