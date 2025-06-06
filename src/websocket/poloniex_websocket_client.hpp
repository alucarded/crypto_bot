#include "serialization_utils.hpp"
#include "websocket/websocket_client.hpp"

#include "json/json.hpp"

#include <string>

using json = nlohmann::json;

class PoloniexWebsocketClient : public WebsocketClient {
public:
  PoloniexWebsocketClient(Consumer<RawTicker>* ticker_consumer) : WebsocketClient(ticker_consumer) {
  }

  virtual inline const std::string GetUrl() const override { return "wss://api2.poloniex.com"; }

  virtual inline const std::string GetConnectionName() const override { return "poloniex"; }

private:
  virtual void request_ticker() override {
      const std::string message = "{\"command\": \"subscribe\", \"channel\": 1002}";
      WebsocketClient::send(message);
  }

  virtual std::optional<RawTicker> extract_ticker(client::message_ptr msg) override {
      auto msg_json = json::parse(msg->get_payload());
      //std::cout << msg->get_payload() << std::endl;
      if (!msg_json.is_array() || msg_json.size() < 3) {
        // std::cout << msg->get_payload() << std::endl;
        return {};
      }
      auto data = msg_json[2];
      if (!data.is_array() || data[0].get<int>() != 121) {
        return {};
      }
      //std::cout << msg->get_payload() << std::endl;
      // TODO: this should be an order book ticker
      RawTicker ticker;
      ticker.bid = data[3];
      ticker.bid_vol = "";
      ticker.ask = data[2];
      ticker.ask_vol = "";
      ticker.source_ts = 0; // not provided
      ticker.exchange = GetConnectionName();
      return std::make_optional(ticker);
  }
};