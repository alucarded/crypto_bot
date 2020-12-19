#include "bitstamp_ticker_client.hpp"
#include "mongo_ticker_consumer.hpp"

#include <iostream>
#include <thread>

// MongoClient("mongodb://" + config["mongo"]["user"] + ":" \
//     + mongo_password["Parameter"]["Value"] + "@" + config["mongo"]["host"] \
//     + ":" + config["mongo"]["port"]  +"/?authSource=" + config["mongo"]["auth_db"])
int main(int argc, char* argv[]) {
    try {
        MongoTickerConsumer mongo_consumer("mongodb://app:DRt99xd4o7PMfygqotE8@3.10.107.166:28888/?authSource=findata", "findata", "CollectorTest");
        BitstampTickerClient bitstamp_client(&mongo_consumer);
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
