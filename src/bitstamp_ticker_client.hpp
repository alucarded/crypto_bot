#include "websocket_client.hpp"

class BitstampTickerClient : public WebsocketClient {
public:
  BitstampTickerClient() {
    WebsocketClient::start("wss://ws.bitstamp.net");
  }

  virtual void on_open(websocketpp::connection_hdl hdl) override {
      std::cout << "Connection opened" << std::endl;
        const std::string message = "{\"event\": \"bts:subscribe\",\"data\": {\"channel\": \"order_book_btcusd\"}}";
        WebsocketClient::send(message);
  }

  virtual void on_message(websocketpp::connection_hdl hdl, client::message_ptr msg) override {
      std::cout << "Got message:" << std::endl;
      std::cout << msg->get_payload() << std::endl;
  }
};