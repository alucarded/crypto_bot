#include "db/mongo_client.hpp"
#include "model/ticker.h"
#include "websocket/websocket_client.hpp"

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
#include <bsoncxx/types.hpp>

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

class MongoTickerConsumer : public ExchangeListener {
public:
    MongoTickerConsumer(MongoClient* mongo_client, const std::string& db_name, const std::string& coll_name)
            : m_mongo_client(mongo_client), m_db_name(db_name), m_coll_name(coll_name) {
        // Create index
        mongocxx::pool::entry client_entry = m_mongo_client->Get();
        mongocxx::client& client = *client_entry;
        auto db = client[m_db_name];
        auto coll = db[m_coll_name];
        document index_builder;
        index_builder << "minute_utc" << 1;
        coll.create_index(index_builder.view());
        mongocxx::options::index index_options{};
        index_options.unique(true);
        document compound_index_builder;
        compound_index_builder << "exchange" << 1
            << "minute_utc" << 1
            << "symbol" << 1
            << "type" << 1;
        coll.create_index(compound_index_builder.view(), index_options);
    }

    virtual void OnConnectionOpen(const std::string& name) override {

    }

    virtual void OnConnectionClose(const std::string& name) override {

    }

    // This can be called from multiple threads
    virtual void OnBookTicker(const Ticker& ticker) {
        BOOST_LOG_TRIVIAL(trace) << "ExchangeListener::OnBookTicker, ticker: " << ticker;
        mongocxx::pool::entry client_entry = m_mongo_client->Get();
        mongocxx::client& client = *client_entry;
        auto db = client[m_db_name];
        auto coll = db[m_coll_name];

        using namespace std::chrono;
        microseconds us = microseconds(ticker.arrived_ts);
        minutes mins = duration_cast<minutes>(us);
        // Save in bucket related to arrival minute (now)
        // Then during backtesting check what was the delay
        // and its impact on order execution.
        std::ostringstream symbol_name_stream;
        symbol_name_stream << SymbolPairId(ticker.symbol);
        coll.update_one(
            document{}
                << "exchange" << ticker.exchange
                << "symbol" << symbol_name_stream.str()
                << "minute_utc" << mins.count()
                // TODO: put all market data stream events in one collection this way
                << "type" << "BOOK_TICKER"
                << finalize,
            document{}
                << "$push" << bsoncxx::builder::stream::open_document << "tickers"
                << bsoncxx::builder::stream::open_document
                << "bid" << double(ticker.bid)
                << "bid_vol" << (ticker.bid_vol.has_value() ? ticker.bid_vol.value() : 0.0)
                << "ask" << ticker.ask
                << "ask_vol" << (ticker.ask_vol.has_value() ? ticker.ask_vol.value() : 0.0)
                // Arrived timestamp in microseconds
                << "a_us" << static_cast<int64_t>(ticker.arrived_ts)
                // Source timestamp in microseconds
                << "s_us" << (ticker.source_ts.has_value() ? ticker.source_ts.value() : 0.0)
                << bsoncxx::builder::stream::close_document
                << bsoncxx::builder::stream::close_document
                << finalize,
            mongocxx::options::update().upsert(true));
    }

    virtual void OnOrderBookUpdate(const OrderBook& order_book) {
        size_t depth = order_book.GetDepth();
        if (depth == 1) {
            OnBookTicker(ExchangeListener::TickerFromOrderBook(order_book));
            return;
        }
        const OrderBookUpdate& ob_update = order_book.GetLastUpdate();
        mongocxx::pool::entry client_entry = m_mongo_client->Get();
        mongocxx::client& client = *client_entry;
        auto db = client[m_db_name];
        auto coll = db[m_coll_name];

        using namespace std::chrono;
        microseconds us = microseconds(ob_update.arrived_ts);
        minutes mins = duration_cast<minutes>(us);
        // Save in bucket related to arrival minute (now)
        // Then during backtesting check what was the delay
        // and its impact on order execution.
        std::ostringstream symbol_name_stream;
        symbol_name_stream << order_book.GetSymbolPairId();
        // Build bids array
        auto push_doc = bsoncxx::builder::basic::document{};
        using bsoncxx::builder::basic::sub_array;
        using bsoncxx::builder::basic::sub_document;
        using bsoncxx::builder::basic::kvp;
        push_doc.append(kvp("$push", [&ob_update](sub_document subdoc) {
            subdoc.append(kvp("updates", [&ob_update](sub_document subdoc2) {
                subdoc2.append(
                    kvp("bids", [&ob_update](sub_array subarr) {
                        for (const auto& bid : ob_update.bids) {
                            subarr.append([&bid](sub_array subarr2) {
                                subarr2.append(bid.price);
                                subarr2.append(bid.volume);
                                if (bid.timestamp.has_value()) {
                                    subarr2.append(static_cast<int64_t>(bid.timestamp.value()));
                                }
                            });
                        }
                }));
                subdoc2.append(
                    kvp("asks", [&ob_update](sub_array subarr) {
                        for (const auto& ask : ob_update.asks) {
                            subarr.append([&ask](sub_array subarr2) {
                                subarr2.append(ask.price);
                                subarr2.append(ask.volume);
                                if (ask.timestamp.has_value()) {
                                    subarr2.append(static_cast<int64_t>(ask.timestamp.value()));
                                }
                            });
                        }
                }));
                    subdoc2.append(kvp("a_us", bsoncxx::types::b_int64{ob_update.arrived_ts}));
                    subdoc2.append(kvp("is_snapshot", bsoncxx::types::b_bool{ob_update.is_snapshot}));
                    subdoc2.append(kvp("last_update_id", bsoncxx::types::b_int64{static_cast<int64_t>(ob_update.last_update_id)}));
            }));
        }));
        BOOST_LOG_TRIVIAL(trace) << "Push: " << bsoncxx::to_json(push_doc.view());
        coll.update_one(
            document{}
                << "exchange" << order_book.GetExchangeName()
                << "symbol" << symbol_name_stream.str()
                << "minute_utc" << mins.count()
                << "type" << "ORDER_BOOK"
                << finalize,
            push_doc.view(),
            mongocxx::options::update().upsert(true));
    }

    virtual void OnTradeTicker(const TradeTicker& ticker) {
        BOOST_LOG_TRIVIAL(debug) << "MongoTickerConsumer::OnTradeTicker, ticker: " << ticker;
        mongocxx::pool::entry client_entry = m_mongo_client->Get();
        mongocxx::client& client = *client_entry;
        auto db = client[m_db_name];
        auto coll = db[m_coll_name];

        using namespace std::chrono;
        microseconds us = microseconds(ticker.arrived_ts);
        minutes mins = duration_cast<minutes>(us);
        // Save in bucket related to arrival minute (now)
        // Then during backtesting check what was the delay
        // and its impact on order execution.
        std::ostringstream symbol_name_stream;
        symbol_name_stream << SymbolPairId(ticker.symbol);
        coll.update_one(
            document{}
                << "exchange" << ticker.exchange
                << "symbol" << symbol_name_stream.str()
                << "minute_utc" << mins.count()
                // TODO: put all market data stream events in one collection this way
                << "type" << "TRADE_TICKER"
                << finalize,
            document{}
                << "$push" << bsoncxx::builder::stream::open_document << "tickers"
                << bsoncxx::builder::stream::open_document
                << "event_time" << int64_t(ticker.event_time)
                << "trade_time" << int64_t(ticker.trade_time)
                << "trade_id" << ticker.trade_id
                << "price" << ticker.price
                << "qty" << ticker.qty
                << "is_market_maker" << ticker.is_market_maker
                // Arrived timestamp in microseconds
                << "a_us" << static_cast<int64_t>(ticker.arrived_ts)
                // Source timestamp in microseconds
                << "s_us" << int64_t(ticker.trade_time)
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