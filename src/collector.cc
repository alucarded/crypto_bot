#include "bitstamp_ticker_client.hpp"

#include <iostream>
#include <thread>

int main(int argc, char* argv[]) {
    try {
        BitstampTickerClient bitstamp_client;
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
