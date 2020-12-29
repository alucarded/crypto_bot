#include "websocket/binance_ticker_client.hpp"
#include "websocket/bitbay_ticker_client.hpp"
#include "websocket/bitstamp_ticker_client.hpp"
#include "websocket/bybit_ticker_client.hpp"
#include "websocket/coinbase_ticker_client.hpp"
#include "websocket/ftx_ticker_client.hpp"
#include "websocket/huobi_global_ticker_client.hpp"
#include "websocket/kraken_ticker_client.hpp"
#include "websocket/okex_ticker_client.hpp"
#include "websocket/poloniex_ticker_client.hpp"
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
                MongoClient::GetInstance()->CreatePool("mongodb://app:DRt99xd4o7PMfygqotE8@3.10.107.166:28888/?authSource=findata");
        MongoTickerConsumer mongo_consumer(mongo_client, "findata", "BtcUsdTicker_v4");
        BinanceTickerClient binance_client(&mongo_consumer);
        // No USDT on Bitstamp
        //BitstampTickerClient bitstamp_client(&mongo_consumer);
        KrakenTickerClient kraken_client(&mongo_consumer);
        CoinbaseTickerClient coinbase_client(&mongo_consumer);
        PoloniexTickerClient poloniex_client(&mongo_consumer);
        BitbayTickerClient bitbay_client(&mongo_consumer);
        HuobiGlobalTickerClient huobi_global_client(&mongo_consumer);
        // OkexTickerClient huobi_global_client(&mongo_consumer);
        // BybitTickerClient bybit_client(&mongo_consumer);
        FtxTickerClient ftx_client(&mongo_consumer);
        std::list<TickerClient*> ticker_client_list = {&binance_client, &bitstamp_client, &kraken_client, &coinbase_client,
                &poloniex_client, &bitbay_client, &huobi_global_client, &ftx_client};
        std::for_each(ticker_client_list.begin(), ticker_client_list.end(), [](TickerClient* tc) { tc->start(); });
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
