#include "serialization_utils.hpp"
#include "websocket/websocket_client.hpp"

#include "json/json.hpp"

#include <string>

using json = nlohmann::json;

class BitstampWebsocketClient : public WebsocketClient {
public:
  BitstampWebsocketClient(Consumer<RawTicker>* ticker_consumer) : WebsocketClient(ticker_consumer) {
  }

  virtual inline const std::string GetUrl() const override { return "wss://ws.bitstamp.net"; }
  virtual inline const std::string GetConnectionName() const override { return "bitstamp"; }

private:
  virtual void request_ticker() override {
      const std::string message = "{\"event\": \"bts:subscribe\",\"data\": {\"channel\": \"order_book_btcusd\"}}";
      WebsocketClient::send(message);
  }

  virtual std::optional<RawTicker> extract_ticker(client::message_ptr msg) override {
      //std::cout << "Got message" << std::endl;
      //std::cout << msg->get_payload() << std::endl;
      auto msg_json = json::parse(msg->get_payload());
      auto data = msg_json["data"];
      if (!utils::check_message(data, {"bids", "asks", "microtimestamp"})) {
        //std::cout << msg->get_payload() << std::endl;
        return {};
      }
      RawTicker ticker;
      ticker.m_bid = data["bids"][0][0];
      ticker.m_bid_vol = data["bids"][0][1];
      ticker.m_ask = data["asks"][0][0];
      ticker.m_ask_vol = data["asks"][0][1];
      ticker.m_source_ts = std::stoull(data["microtimestamp"].get<std::string>());
      ticker.m_exchange = GetConnectionName();
      return std::make_optional(ticker);
  }
};