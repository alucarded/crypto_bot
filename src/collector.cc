#include "websocket_client.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    std::string uri = "wss://ws.bitstamp.net";
    std::string input;

    if (argc == 2) {
        uri = argv[1];
    }

    try {
        WebsocketClient endpoint;
        endpoint.start(uri);
        std::getline(std::cin, input);
        std::string message = "{\"event\": \"bts:subscribe\",\"data\": {\"channel\": \"order_book_btcusd\"}}";
        endpoint.send(message);
        std::getline(std::cin, input);
    } catch (websocketpp::exception const & e) {
        std::cout << e.what() << std::endl;
    } catch (std::exception const & e) {
        std::cout << e.what() << std::endl;
    } catch (...) {
        std::cout << "other exception" << std::endl;
    }
}
