#include "consumer/ticker_consumer.h"
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

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

class MongoTickerProducer {
public:
  MongoTickerProducer(MongoClient* mongo_client, const std::string& db_name,
      const std::string &coll_name, TickerConsumer* ticker_consumer)
      : m_mongo_client(mongo_client), m_db_name(db_name), m_coll_name(coll_name), m_ticker_consumer(ticker_consumer) {

  }

  void Produce(const int64_t from_min, const int64_t to_min) {
    mongocxx::pool::entry client_entry = m_mongo_client->Get();
    mongocxx::client& client = *client_entry;
    auto db = client[m_db_name];
    auto coll = db[m_coll_name];
    // { minute_utc: { $gte: 26813490, $lt: 26813500 } }
    mongocxx::cursor cursor = coll.find(document{} << "minute_utc"
        << open_document
        << "$gte" << from_min
        << "$lt" << to_min
        << close_document << finalize);
    for(auto doc : cursor) {
      std::cout << bsoncxx::to_json(doc) << "\n";
      RawTicker raw_ticker;
      if (m_ticker_consumer) {
        m_ticker_consumer->Consume(raw_ticker);
      }
    }
  }

private:
  MongoClient* m_mongo_client;
  const std::string m_db_name;
  const std::string m_coll_name;
  TickerConsumer* m_ticker_consumer;
};