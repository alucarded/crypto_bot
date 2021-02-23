#include "websocket/binance_websocket_client.hpp"
#include "websocket/bitbay_websocket_client.hpp"
#include "websocket/bitstamp_websocket_client.hpp"
#include "websocket/bybit_websocket_client.hpp"
#include "websocket/coinbase_websocket_client.hpp"
#include "websocket/ftx_websocket_client.hpp"
#include "websocket/huobi_global_websocket_client.hpp"
#include "websocket/kraken_websocket_client.hpp"
#include "websocket/okex_websocket_client.hpp"
#include "websocket/poloniex_websocket_client.hpp"
#include "consumer/mongo_ticker_consumer.hpp"
#include "db/mongo_client.hpp"
#include "utils/config.hpp"

#include <iostream>
#include <thread>

int main(int argc, char* argv[]) {
    if (argc < 2) {
      std::cerr << "Please provide configuration file" << std::endl;
      return -1;
    }
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

    MongoTickerConsumer mongo_consumer(mongo_client, "findata", "BtcUsdtTicker_v2");
    // TODO: add Binance US for USDT-USD rates
    BinanceWebsocketClient binance_client(&mongo_consumer);
    // No USDT on Bitstamp
    //BitstampWebsocketClient bitstamp_client(&mongo_consumer);
    KrakenWebsocketClient kraken_client(&mongo_consumer);
    // No USDT on Coinbase
    //CoinbaseWebsocketClient coinbase_client(&mongo_consumer);
    //PoloniexWebsocketClient poloniex_client(&mongo_consumer);
    //BitbayWebsocketClient bitbay_client(&mongo_consumer);
    //HuobiGlobalWebsocketClient huobi_global_client(&mongo_consumer);
    // OkexWebsocketClient huobi_global_client(&mongo_consumer);
    // BybitWebsocketClient bybit_client(&mongo_consumer);
    //FtxWebsocketClient ftx_client(&mongo_consumer);
    std::list<WebsocketClient*> websocket_client_list = {&binance_client,
            &kraken_client//,
            //&poloniex_client,
            //&bitbay_client,
            //&huobi_global_client,
            //&ftx_client
    };

    try {
        std::for_each(websocket_client_list.begin(), websocket_client_list.end(), [](WebsocketClient* tc) { tc->start(); });
        // Just wait for now
        std::this_thread::sleep_until(std::chrono::time_point<std::chrono::system_clock>::max());
    } catch (websocketpp::exception const & e) {
        std::cout << e.what() << std::endl;
    } catch (std::exception const & e) {
        std::cout << e.what() << std::endl;
    } catch (...) {
        std::cout << "other exception" << std::endl;
    }
}
