#include "serialization_utils.hpp"
#include "websocket/websocket_client.hpp"

#include "json/json.hpp"

#include <chrono>
#include <string>

using json = nlohmann::json;

// TODO: connection is disconnected after 24 hours - need to re-connect
class BinanceWebsocketClient : public WebsocketClient {
public:
  BinanceWebsocketClient(Consumer<RawTicker>* ticker_consumer) : WebsocketClient(ticker_consumer) {
  }

  virtual inline const std::string GetUrl() const override { return "wss://fstream.binance.com/ws/bookTicker"; }
  virtual inline const std::string GetConnectionName() const override { return "binance"; }

private:
  virtual void request_ticker() override {
      const std::string message = "{\"method\": \"SUBSCRIBE\",\"params\": [\"btcusdt@bookTicker\"],\"id\": 1}";
      WebsocketClient::send(message);
  }

  virtual std::optional<RawTicker> extract_ticker(client::message_ptr msg) override {
      //std::cout << msg->get_payload() << std::endl;
      auto msg_json = json::parse(msg->get_payload());
      // TODO: Perhaps do not do schema validation for every message in production
      if (!msg_json.is_object()
          || !utils::check_message(msg_json, {"u", "s", "b", "B", "a", "A", "T"})
          || msg_json["s"].get<std::string>() != "BTCUSDT") {
        std::cout << "Binance: Not an expected ticker object" << std::endl;
        return {};
      }
      RawTicker ticker;
      ticker.m_bid = msg_json["b"];
      ticker.m_bid_vol = msg_json["B"];
      ticker.m_ask = msg_json["a"];
      ticker.m_ask_vol = msg_json["A"];
      // Transaction time (received in ms)
      ticker.m_source_ts = msg_json["T"].get<int64_t>() * 1000;
      // TODO: perhaps generate timestamp in base class and pass it to this method
      using namespace std::chrono;
      auto now = system_clock::now();
      system_clock::duration tp = now.time_since_epoch();
      microseconds us = duration_cast<microseconds>(tp);
      ticker.m_arrived_ts = us.count();
      ticker.m_exchange = GetConnectionName();
      // TODO: this should be an internally common enum
      ticker.m_symbol = msg_json["s"];
      return std::make_optional(ticker);
  }
};