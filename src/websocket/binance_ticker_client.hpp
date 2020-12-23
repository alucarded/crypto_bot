#include "serialization_utils.hpp"
#include "websocket/ticker_client.hpp"

#include "json/json.hpp"

#include <string>

using json = nlohmann::json;

// TODO: connection is disconnected after 24 hours - need to re-connect
class BinanceTickerClient : public TickerClient {
public:
  BinanceTickerClient(TickerConsumer* ticker_consumer) : TickerClient(ticker_consumer) {
  }

  virtual inline const std::string GetUrl() const override { return "wss://stream.binance.com:9443/ws/bookTicker"; }
  virtual inline const std::string GetExchangeName() const override { return "binance"; }

private:
  virtual void request_ticker() override {
      const std::string message = "{\"method\": \"SUBSCRIBE\",\"params\": [\"btcusdt@bookTicker\"],\"id\": 1}";
      TickerClient::send(message);
  }

  virtual std::optional<RawTicker> extract_ticker(client::message_ptr msg) override {
      //std::cout << msg->get_payload() << std::endl;
      auto msg_json = json::parse(msg->get_payload());
      // TODO: Perhaps do not do schema validation for every message in production
      if (!msg_json.is_object()
          || !utils::check_message(msg_json, {"u", "s", "b", "B", "a", "A"})
          || msg_json["s"].get<std::string>() != "BTCUSDT") {
        std::cout << "Binance: Not an expected ticker object" << std::endl;
        return {};
      }
      RawTicker ticker;
      ticker.m_bid = msg_json["b"];
      ticker.m_bid_vol = msg_json["B"];
      ticker.m_ask = msg_json["a"];
      ticker.m_ask_vol = msg_json["A"];
      ticker.m_source_ts = 0; // not provided
      ticker.m_exchange = GetExchangeName();
      ticker.m_symbol = msg_json["s"];
      return std::make_optional(ticker);
  }
};