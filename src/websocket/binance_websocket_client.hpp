#include "serialization_utils.hpp"
#include "websocket/websocket_client.hpp"

#include "json/json.hpp"

#include <chrono>
#include <optional>
#include <string>

using json = nlohmann::json;

// TODO: connection is disconnected after 24 hours - need to re-connect
class BinanceWebsocketClient : public WebsocketClient {
public:
  inline static const std::string NAME = "binance";

  BinanceWebsocketClient(ExchangeListener* exchange_listener) : WebsocketClient("wss://fstream.binance.com/ws/bookTicker", NAME), m_exchange_listener(exchange_listener) {
  }

  void SubscribeTicker(const std::string& symbol) {
      const std::string message = "{\"method\": \"SUBSCRIBE\",\"params\": [\"" + symbol + "@bookTicker\"],\"id\": 1}";
      m_subscription_msg.push_back(message);
      WebsocketClient::send(message);
  }

private:
  virtual void OnOpen(websocketpp::connection_hdl conn) override {
    for (const auto& msg : m_subscription_msg) {
      WebsocketClient::send(msg);
    }
    m_exchange_listener->OnConnectionOpen(NAME);
  }

  virtual void OnClose(websocketpp::connection_hdl conn) override {
    m_exchange_listener->OnConnectionClose(NAME);
  }

  virtual void OnMessage(websocketpp::connection_hdl, client::message_ptr msg) override {
      //std::cout << msg->get_payload() << std::endl;
      auto msg_json = json::parse(msg->get_payload());
      // TODO: Perhaps do not do schema validation for every message in production
      if (!msg_json.is_object()
          || !utils::check_message(msg_json, {"u", "s", "b", "B", "a", "A", "T"})
          || msg_json["s"].get<std::string>() != "BTCUSDT") {
        std::cout << "Binance: Not an expected ticker object" << std::endl;
        return;
      }
      Ticker ticker;
      ticker.m_bid = std::stod(msg_json["b"].get<std::string>());
      ticker.m_bid_vol = std::optional<double>(std::stod(msg_json["B"].get<std::string>()));
      ticker.m_ask = std::stod(msg_json["a"].get<std::string>());
      ticker.m_ask_vol = std::optional<double>(std::stod(msg_json["A"].get<std::string>()));
      // Transaction time (received in ms)
      ticker.m_source_ts = std::optional<int64_t>(msg_json["T"].get<int64_t>() * 1000);
      // TODO: perhaps generate timestamp in base class and pass it to this method
      using namespace std::chrono;
      auto now = system_clock::now();
      system_clock::duration tp = now.time_since_epoch();
      microseconds us = duration_cast<microseconds>(tp);
      ticker.m_arrived_ts = us.count();
      ticker.m_exchange = NAME;
      // TODO: this should be an internally common enum
      ticker.m_symbol = msg_json["s"];
      // TODO: should be unique across exchange
      ticker.m_id = 1;
      m_exchange_listener->OnTicker(ticker);
  }

private:
  ExchangeListener* m_exchange_listener;
  std::vector<std::string> m_subscription_msg;
};