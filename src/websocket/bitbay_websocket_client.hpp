#include "serialization_utils.hpp"
#include "websocket/websocket_client.hpp"

#include "json/json.hpp"

#include <string>

using json = nlohmann::json;

// TODO: try re-opening connection on close, routing policy might change few time per day
class BitbayWebsocketClient : public WebsocketClient {
public:
  BitbayWebsocketClient(Consumer<RawTicker>* ticker_consumer) : WebsocketClient(ticker_consumer) {
  }

  virtual inline const std::string GetUrl() const override { return "wss://api.bitbay.net/websocket/"; }
  virtual inline const std::string GetConnectionName() const override { return "bitbay"; }

private:
  virtual void request_ticker() override {
      const std::string message = "{\"action\": \"subscribe-public\",\"module\": \"trading\",\"path\": \"ticker/BTC-USDT\"}";
      WebsocketClient::send(message);
  }

  virtual std::optional<RawTicker> extract_ticker(client::message_ptr msg) override {
      auto msg_json = json::parse(msg->get_payload());
      //std::cout << msg->get_payload() << std::endl;
      if (!msg_json.is_object()
          || !msg_json.contains("topic")
          || msg_json["topic"].get<std::string>() != "trading/ticker/btc-usdt") {
        // std::cout << msg->get_payload() << std::endl;
        return {};
      }
      auto message = msg_json["message"];
      // TODO: this should be an order book ticker
      RawTicker ticker;
      ticker.m_bid = message["highestBid"];
      ticker.m_bid_vol = "";
      ticker.m_ask = message["lowestAsk"];
      ticker.m_ask_vol = "";
      ticker.m_source_ts = std::stoull(message["time"].get<std::string>()) * 1000;
      ticker.m_exchange = GetConnectionName();
      return std::make_optional(ticker);
  }
};