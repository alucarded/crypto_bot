#include "db/mongo_client.hpp"
#include "exchange/connection_listener.h"
#include "exchange/exchange_listener.h"
#include "model/order_book_update.h"
#include "model/order_book.h"
#include "model/real_time_event.h"
#include "model/ticker.h"
#include "model/trade_ticker.h"
#include "utils/string.h"

#include <boost/log/trivial.hpp>

#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/instance.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>

#include <cassert>
#include <iterator>
#include <limits>
#include <variant>
#include <vector>

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

// Produces book tickers
class MongoTickerProducer {
public:
  typedef std::vector<std::variant<Ticker, TradeTicker, OrderBookUpdate>> EventsVector;

  MongoTickerProducer(MongoClient* mongo_client, const std::string& db_name,
      const std::string &coll_name)
      : m_mongo_client(mongo_client), m_db_name(db_name), m_coll_name(coll_name) {

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
    mongocxx::options::find opts;
    // It is best to also make sure there is an ascending index for minute_utc
    opts.sort(document{} << "minute_utc" << 1 << finalize);
    // Get Mongo cursor
    mongocxx::cursor cursor = coll.find(document{} << "minute_utc"
        << open_document
        << "$gte" << from_min
        << "$lt" << to_min
        << close_document << finalize,
        opts);
    Produce(cursor);
  }

  int64_t Produce() {
    mongocxx::pool::entry client_entry = m_mongo_client->Get();
    mongocxx::client& client = *client_entry;
    auto db = client[m_db_name];
    auto coll = db[m_coll_name];
    mongocxx::options::find opts;
    // It is best to also make sure there is an ascending index for minute_utc
    opts.sort(document{} << "minute_utc" << 1 << finalize);
    // Get Mongo cursor
    mongocxx::cursor cursor = coll.find({}, opts);
    return Produce(cursor);
  }

  void Register(ExchangeListener* exchange_listener) {
    m_exchange_listeners.push_back(exchange_listener);
  }

  void Unregister(ExchangeListener* exchange_listener) {
    size_t i = 0;
    while (i < m_exchange_listeners.size()) {
      if (m_exchange_listeners.at(i) == exchange_listener) {
        m_exchange_listeners.erase(m_exchange_listeners.begin() + i);
      } else {
        ++i;
      }
    }
  }
private:

  int64_t Produce(mongocxx::cursor& cursor) {
    // Put all tickers in a vector
    EventsVector events_vec;
    // std::holds_alternative<std::string>(v)
    int64_t prev_min_bucket = 0;
    int64_t total_tickers = 0;
    for(auto doc : cursor) {
      // std::getchar();
      // std::cout << bsoncxx::to_json(doc) << "\n";
      bsoncxx::document::element minute_utc_elem = doc["minute_utc"];
      int64_t minute_utc = minute_utc_elem.get_int64().value;
      if (prev_min_bucket == 0) {
        prev_min_bucket = minute_utc;
      }
      if (minute_utc > prev_min_bucket) {
        total_tickers += events_vec.size();
        BOOST_LOG_TRIVIAL(info) << "Flushing tickers for minute " << prev_min_bucket << "... " << total_tickers;
        FlushEvents(events_vec);
      }

      //bsoncxx::document::element exchange_elem = doc["exchange"];
      bsoncxx::document::element type_elem = doc["type"];
      const std::string& type = type_elem.get_value().get_utf8().value.to_string();

      if (type == "BOOK_TICKER") {
        PushBookTickers(doc, events_vec);
      } else if (type == "TRADE_TICKER") {
        PushTradeTickers(doc, events_vec);
      } else if (type == "ORDER_BOOK") {
        PushOrderBookUpdates(doc, events_vec);
      } else {
        BOOST_LOG_TRIVIAL(error) << "Unsupported event";
      }

      assert(minute_utc >= prev_min_bucket);
      prev_min_bucket = minute_utc;
    }
    return total_tickers;
    
  }

  void FlushEvents(EventsVector& events_vec) {
    // std::sort(events_vec.begin(), events_vec.end(), [](const auto& lhs, const auto& rhs) -> bool {
    //   // Here we assume ticks always arrive in the right order
    //   // In strategy part we can verify it and compare source timestamps and arrived timestamps
    //   return a.arrived_ts < b.arrived_ts;
    // });
    std::sort(std::begin(events_vec), std::end(events_vec),
    [](auto const& lhs, auto const& rhs)
    {
        return std::visit([](auto const& x, auto const& y){
            return x.arrived_ts < y.arrived_ts;
        }, lhs, rhs);
    });
    std::for_each(events_vec.begin(), events_vec.end(), [&](const auto& obj) {
      for (const auto& listener_ptr : m_exchange_listeners) {
        if (std::holds_alternative<Ticker>(obj)) {
          listener_ptr->OnBookTicker(std::get<Ticker>(obj));
        } else if (std::holds_alternative<TradeTicker>(obj)) {
          listener_ptr->OnTradeTicker(std::get<TradeTicker>(obj));
        } else if (std::holds_alternative<OrderBookUpdate>(obj)) {
          const auto& ob_update = std::get<OrderBookUpdate>(obj);
          OrderBook& ob = GetOrCreateOrderBook(ob_update);
          ob.Update(ob_update);
          listener_ptr->OnOrderBookUpdate(ob);
        } else {
          BOOST_LOG_TRIVIAL(error) << "Unsupported event in events vector. Impossible!";
        }
      }
    });
    events_vec.clear();
  }

  void PushBookTickers(const bsoncxx::document::view& doc, EventsVector& events_vec) {
    bsoncxx::document::element exchange_elem = doc["exchange"];
    SymbolPairId symbol = GetSymbol(doc);
    bsoncxx::document::element tickers_elem = doc["tickers"];
    bsoncxx::types::b_array arr = tickers_elem.get_array();

    for (bsoncxx::array::element ticker_doc : arr.value) {
      Ticker ticker;
      if (ticker_doc["bid"].type() != bsoncxx::type::k_double) {
        BOOST_LOG_TRIVIAL(warning) << "Bid value does not have double type! The type is " << int(ticker_doc["bid"].type());
        continue;
      }
      ticker.bid = ticker_doc["bid"].get_double();
      ticker.bid_vol = ticker_doc["bid_vol"].get_double();
      ticker.ask = ticker_doc["ask"].get_double();
      ticker.ask_vol = ticker_doc["ask_vol"].get_double();
      // if (ticker_doc["s_us"].type() == bsoncxx::type::k_int64) {
      //   ticker.source_ts = ticker_doc["s_us"].get_int64().value;
      // } else {
      //   BOOST_LOG_TRIVIAL(debug) << "Source timestamp value does not have int64 type! The type is " << int(ticker_doc["s_us"].type());
      // }
      ticker.source_ts = std::nullopt;
      ticker.arrived_ts = static_cast<uint64_t>(ticker_doc["a_us"].get_int64().value);
      ticker.exchange = exchange_elem.get_value().get_utf8().value.to_string();
      ticker.symbol = symbol;
      //BOOST_LOG_TRIVIAL(info) << "Ticker: " << ticker;
      events_vec.push_back(ticker);
      // std::cout << "Added ticker:" << std::endl;
      // std::cout << ticker;
    }
  }

  void PushTradeTickers(const bsoncxx::document::view& doc, EventsVector& events_vec) {
    bsoncxx::document::element exchange_elem = doc["exchange"];
    std::string exchange = exchange_elem.get_value().get_utf8().value.to_string();
    SymbolPairId symbol = GetSymbol(doc);
    bsoncxx::document::element tickers_elem = doc["tickers"];
    bsoncxx::types::b_array arr = tickers_elem.get_array();

    for (bsoncxx::array::element ticker_doc : arr.value) {
      TradeTicker trade_ticker;
      trade_ticker.symbol = symbol;
      trade_ticker.exchange = exchange;
      trade_ticker.event_time = static_cast<uint64_t>(ticker_doc["event_time"].get_int64().value);
      trade_ticker.trade_time = static_cast<uint64_t>(ticker_doc["trade_time"].get_int64().value);
      trade_ticker.trade_id = ticker_doc["trade_id"].get_utf8().value.to_string();
      trade_ticker.price = ticker_doc["price"].get_double();
      trade_ticker.qty = ticker_doc["qty"].get_double();
      trade_ticker.is_market_maker = ticker_doc["is_market_maker"].get_bool();
      trade_ticker.arrived_ts = static_cast<uint64_t>(ticker_doc["a_us"].get_int64().value);
      events_vec.push_back(trade_ticker);
    }
  }

  void PushOrderBookUpdates(const bsoncxx::document::view& doc, EventsVector& events_vec) {
    bsoncxx::document::element exchange_elem = doc["exchange"];
    const auto& exchange = exchange_elem.get_value().get_utf8().value.to_string();
    SymbolPairId symbol = GetSymbol(doc);
    bsoncxx::types::b_array updates_arr = doc["updates"].get_array();

    for (bsoncxx::array::element update_doc : updates_arr.value) {
      OrderBookUpdate ob_update;
      ob_update.exchange = exchange;
      ob_update.symbol = symbol;
      bsoncxx::array::view bids_arr = update_doc["bids"].get_array();
      for (bsoncxx::array::element bid_doc : bids_arr) {
        bsoncxx::array::view bid_arr = bid_doc.get_array();
        OrderBookUpdate::Level level;
        level.price = bid_arr[0].get_utf8().value.to_string();
        level.volume = bid_arr[1].get_utf8().value.to_string();
        if (std::distance(bid_arr.begin(), bid_arr.end()) > 2) {
          level.timestamp = static_cast<uint64_t>(bid_arr[2].get_int64().value);
        }
        ob_update.bids.push_back(level);
      }
      bsoncxx::array::view asks_arr = update_doc["asks"].get_array();
      for (bsoncxx::array::element ask_doc : asks_arr) {
        bsoncxx::array::view ask_arr = ask_doc.get_array();
        OrderBookUpdate::Level level;
        level.price = ask_arr[0].get_utf8().value.to_string();
        level.volume = ask_arr[1].get_utf8().value.to_string();
        if (std::distance(ask_arr.begin(), ask_arr.end()) > 2) {
          level.timestamp = static_cast<uint64_t>(ask_arr[2].get_int64().value);
        }
        ob_update.asks.push_back(level);
      }
      ob_update.last_update_id = static_cast<uint64_t>(update_doc["last_update_id"].get_int64().value);
      ob_update.is_snapshot = update_doc["is_snapshot"].get_bool();
      ob_update.arrived_ts = static_cast<uint64_t>(update_doc["a_us"].get_int64().value);
      events_vec.push_back(ob_update);
    }
  }

  SymbolPairId GetSymbol(const bsoncxx::document::view& doc) const {
    if (doc["symbol"].type() == bsoncxx::type::k_int32) {
      return SymbolPairId(doc["symbol"].get_int32().value);
    } else if (doc["symbol"].type() == bsoncxx::type::k_int64) { 
      return SymbolPairId(doc["symbol"].get_int64().value);
    } else if (doc["symbol"].type() == bsoncxx::type::k_utf8) {
      return SymbolPair(doc["symbol"].get_utf8().value.to_string());
    } else {
      BOOST_LOG_TRIVIAL(warning) << "Symbol value has type: " << int(doc["symbol"].type());
      return SymbolPairId::UNKNOWN;
    }
  }

  OrderBook& GetOrCreateOrderBook(const OrderBookUpdate& update) {
    for (auto& ob : m_order_books) {
      if (ob.GetExchangeName() == update.exchange && ob.GetSymbolPairId() == update.symbol) {
        return ob;
      }
    }

    // TODO: configurable depth
    m_order_books.emplace_back(update.exchange, update.symbol, 1000, PrecisionSettings(cryptobot::precision_from_string(update.bids[0].price), cryptobot::precision_from_string(update.bids[0].volume), 3));
    return m_order_books[m_order_books.size() - 1];
  }

  MongoClient* m_mongo_client;
  const std::string m_db_name;
  const std::string m_coll_name;
  std::vector<OrderBook> m_order_books;
  std::vector<ExchangeListener*> m_exchange_listeners;
};