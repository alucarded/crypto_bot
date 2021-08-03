#include "http/binance_client.hpp"
#include "websocket/binance_book_ticker_stream.hpp"
#include "websocket/binance_order_book_stream.hpp"
#include "websocket/binance_trade_ticker_stream.hpp"
#include "websocket/kraken_websocket_client.hpp"
#include "db/mongo_ticker_consumer.hpp"
#include "db/mongo_client.hpp"
#include "utils/config.hpp"

// TODO: which includes are really neccessary?
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include <iostream>
#include <thread>

namespace logging = boost::log;
namespace keywords = boost::log::keywords;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Please provide configuration file" << std::endl;
        return -1;
    }
    logging::core::get()->set_filter(
        logging::trivial::severity >= logging::trivial::info);
    logging::add_common_attributes();

    std::string config_path(argv[1]);
    auto config_json = cryptobot::GetConfigJson(config_path);
    // TODO: take credentials from parameter store
    std::string pass("DRt99xd4o7PMfygqotE8");
    MongoClient* mongo_client =
            MongoClient::GetInstance()->CreatePool("mongodb://" + config_json["user"].get<std::string>() + ":" + pass
            + "@" + config_json["host"].get<std::string>() + ":" + config_json["port"].get<std::string>()
            + "/?authSource=" + config_json["authSource"].get<std::string>());
    std::cout << "Created Mongo client pool" << std::endl;
    pass.clear();

    MongoTickerConsumer mongo_consumer(mongo_client, config_json["db"].get<std::string>(), config_json["collection"].get<std::string>());

    BinanceBookTickerStream binance_book_ticker_stream(&mongo_consumer);
    std::promise<void> binance_book_ticker_promise;
    std::future<void> binance_book_ticker_future = binance_book_ticker_promise.get_future();
    binance_book_ticker_stream.set_do_reconnect(false);
    binance_book_ticker_stream.start(std::move(binance_book_ticker_promise));
    binance_book_ticker_future.wait();

    binance_book_ticker_stream.SubscribeTicker("adausdt");

    BinanceTradeTickerStream binance_trade_ticker_stream(&mongo_consumer);
    std::promise<void> binance_trade_ticker_promise;
    std::future<void> binance_trade_ticker_future = binance_trade_ticker_promise.get_future();
    binance_trade_ticker_stream.set_do_reconnect(false);
    binance_trade_ticker_stream.start(std::move(binance_trade_ticker_promise));
    binance_trade_ticker_future.wait();

    binance_trade_ticker_stream.SubscribeTicker("adausdt");
    binance_trade_ticker_stream.SubscribeTicker("btcusdt");
    binance_trade_ticker_stream.SubscribeTicker("ethusdt");
    binance_trade_ticker_stream.SubscribeTicker("bnbusdt");

    BinanceClient binance_client;
    BinanceSettings binance_settings = binance_client.GetBinanceSettings();
    // TODO: specify pair from config
    BinanceOrderBookStream binance_order_book_stream(binance_settings.GetPairSettings(SymbolPairId::ADA_USDT), &binance_client, &mongo_consumer);
    std::promise<void> binance_order_book_promise;
    std::future<void> binance_order_book_future = binance_order_book_promise.get_future();
    binance_order_book_stream.set_do_reconnect(false);
    binance_order_book_stream.start(std::move(binance_order_book_promise));

    // Just sleep
    std::this_thread::sleep_until(std::chrono::time_point<std::chrono::system_clock>::max());
}
