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

#include <iostream>
#include <thread>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    try {
        // TODO: take credentials from parameter store ?
        MongoClient* mongo_client =
                MongoClient::GetInstance()->CreatePool("mongodb://app:DRt99xd4o7PMfygqotE8@18.132.211.87:28888/?authSource=findata");
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
