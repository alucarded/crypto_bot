#include "exchange/exchange_listener.h"
#include "serialization_utils.hpp"
#include "websocket/websocket_client.hpp"

#include "json/json.hpp"

#include <string>

using json = nlohmann::json;

class KrakenWebsocketClient : public WebsocketClient {
public:
  inline static const std::string NAME = "kraken";

  KrakenWebsocketClient(ExchangeListener* exchange_listener) : WebsocketClient("wss://beta-ws.kraken.com", NAME), m_exchange_listener(exchange_listener) {
  }

  void RequestTicker(const std::string& symbol) {
    const std::string message = "{\"event\": \"subscribe\",\"pair\": [\"" + symbol + "\"],\"subscription\": {\"name\": \"ticker\"}}";
    WebsocketClient::send(message);
  }

  void RequestOrderBook(const std::string& symbol) {
    const std::string message = "{\"event\": \"subscribe\",\"pair\": [\"" + symbol + "\"],\"subscription\": {\"name\": \"book\",\"depth\": 10}}";
    WebsocketClient::send(message);
  }

private:

  virtual void OnOpen(websocketpp::connection_hdl conn) override {
    m_exchange_listener->OnConnectionOpen(NAME);
  }

  virtual void OnClose(websocketpp::connection_hdl conn) override {
    m_exchange_listener->OnConnectionClose(NAME);
  }

  virtual void OnMessage(websocketpp::connection_hdl, client::message_ptr msg) override {
    auto msg_json = json::parse(msg->get_payload());
    //std::cout << msg_json << std::endl;
    if (!msg_json.is_array()) {
      // It is most probably a publication or response
      // TODO:
      return;
    }
    auto channel_name = msg_json[2];
    if (channel_name == "ticker") {
      m_exchange_listener->OnTicker(RawTicker::ToTicker(ParseTicker(msg_json[1])));
    } else if (channel_name.get<std::string>().find("book") != std::string::npos) {
      auto book_obj = msg_json[1];
      if (book_obj.contains("as")) {
        OnOrderBookSnapshot(book_obj);
      } else {
        OnOrderBookUpdate(book_obj);
      }
      m_exchange_listener->OnOrderBookUpdate(m_order_book);
    }
  }

  RawTicker ParseTicker(json data) {
      RawTicker ticker;
      ticker.m_bid = data["b"][0];
      ticker.m_bid_vol = data["b"][2];
      ticker.m_ask = data["a"][0];
      ticker.m_ask_vol = data["a"][2];
      ticker.m_source_ts = 0; // not provided
      // TODO: perhaps generate timestamp in base class and pass it to this method
      using namespace std::chrono;
      auto now = system_clock::now();
      system_clock::duration tp = now.time_since_epoch();
      microseconds us = duration_cast<microseconds>(tp);
      ticker.m_arrived_ts = us.count();
      ticker.m_exchange = NAME;
      // TODO: use enum
      ticker.m_symbol = "BTCUSDT";
      return ticker;
  }

  void OnOrderBookSnapshot(const json& snapshot_obj) {
    // TODO:
  }

  void OnOrderBookUpdate(const json& update_obj) {
    // TODO:
  }

private:
  ExchangeListener* m_exchange_listener;
  OrderBook m_order_book;
};