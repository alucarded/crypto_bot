#include "serialization_utils.hpp"
#include "websocket/ticker_client.hpp"

#include "json/json.hpp"

#include <string>

using json = nlohmann::json;

// TODO: try re-opening connection on close, routing policy might change few time per day
class BitbayTickerClient : public TickerClient {
public:
  BitbayTickerClient(TickerConsumer* ticker_consumer) : TickerClient(ticker_consumer) {
  }

  virtual inline const std::string GetUrl() const override { return "wss://api.bitbay.net/websocket/"; }
  virtual inline const std::string GetExchangeName() const override { return "bitbay"; }

private:
  virtual void request_ticker() override {
      const std::string message = "{\"action\": \"subscribe-public\",\"module\": \"trading\",\"path\": \"ticker/BTC-USD\"}";
      TickerClient::send(message);
  }

  virtual std::optional<RawTicker> extract_ticker(client::message_ptr msg) override {
      auto msg_json = json::parse(msg->get_payload());
      //std::cout << msg->get_payload() << std::endl;
      if (!msg_json.is_object()
          || !msg_json.contains("topic")
          || msg_json["topic"].get<std::string>() != "trading/ticker/btc-usd") {
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
      ticker.m_exchange = GetExchangeName();
      return std::make_optional(ticker);
  }
};