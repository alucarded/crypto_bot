#include "websocket/binance_websocket_client.hpp"
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
    BinanceWebsocketClient binance_websocket_client(&mongo_consumer);
    KrakenWebsocketClient kraken_websocket_client(&mongo_consumer);

    std::promise<void> binance_promise;
    std::future<void> binance_future = binance_promise.get_future();
    binance_websocket_client.start(std::move(binance_promise));

    std::promise<void> kraken_promise;
    std::future<void> kraken_future = kraken_promise.get_future();
    kraken_websocket_client.start(std::move(kraken_promise));

    binance_future.wait();
    kraken_future.wait();

    auto wait_time = 400ms;
    binance_websocket_client.SubscribeTicker("btcusdt");
    std::this_thread::sleep_for(wait_time);
    binance_websocket_client.SubscribeTicker("adausdt");
    std::this_thread::sleep_for(wait_time);
    // binance_websocket_client.SubscribeTicker("dotusdt");
    // std::this_thread::sleep_for(wait_time);
    // binance_websocket_client.SubscribeTicker("eosusdt");
    // std::this_thread::sleep_for(wait_time);
    // binance_websocket_client.SubscribeTicker("ethusdt");
    // std::this_thread::sleep_for(wait_time);
    binance_websocket_client.SubscribeTicker("adabtc");
    // std::this_thread::sleep_for(wait_time);
    // binance_websocket_client.SubscribeTicker("dotbtc");
    // std::this_thread::sleep_for(wait_time);
    // binance_websocket_client.SubscribeTicker("eosbtc");
    // std::this_thread::sleep_for(wait_time);
    // binance_websocket_client.SubscribeTicker("eoseth");
    // std::this_thread::sleep_for(wait_time);
    // binance_websocket_client.SubscribeTicker("ethbtc");
    // std::this_thread::sleep_for(wait_time);
    // binance_websocket_client.SubscribeTicker("xlmbtc");

    kraken_websocket_client.SubscribeOrderBook("XBT/USDT");
    std::this_thread::sleep_for(wait_time);
    kraken_websocket_client.SubscribeOrderBook("ADA/USDT");
    std::this_thread::sleep_for(wait_time);
    // kraken_websocket_client.SubscribeOrderBook("DOT/USDT");
    // std::this_thread::sleep_for(wait_time);
    // kraken_websocket_client.SubscribeOrderBook("EOS/USDT");
    // std::this_thread::sleep_for(wait_time);
    // kraken_websocket_client.SubscribeOrderBook("ETH/USDT");
    // std::this_thread::sleep_for(wait_time);
    kraken_websocket_client.SubscribeOrderBook("ADA/XBT");
    // std::this_thread::sleep_for(wait_time);
    // kraken_websocket_client.SubscribeOrderBook("DOT/XBT");
    // std::this_thread::sleep_for(wait_time);
    // kraken_websocket_client.SubscribeOrderBook("EOS/XBT");
    // std::this_thread::sleep_for(wait_time);
    // kraken_websocket_client.SubscribeOrderBook("EOS/ETH");
    // std::this_thread::sleep_for(wait_time);
    // kraken_websocket_client.SubscribeOrderBook("ETH/XBT");
    // std::this_thread::sleep_for(wait_time);
    // kraken_websocket_client.SubscribeOrderBook("XLM/XBT");

    // Just wait for now
    std::this_thread::sleep_until(std::chrono::time_point<std::chrono::system_clock>::max());
}
