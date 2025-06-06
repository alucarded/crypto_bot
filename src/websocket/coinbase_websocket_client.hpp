#include "serialization_utils.hpp"
#include "websocket/websocket_client.hpp"

#include "json/json.hpp"

#include <string>

using json = nlohmann::json;

class CoinbaseWebsocketClient : public WebsocketClient {
public:
  CoinbaseWebsocketClient(Consumer<RawTicker>* ticker_consumer) : WebsocketClient(ticker_consumer) {
  }

  virtual inline const std::string GetUrl() const override { return "wss://ws-feed.pro.coinbase.com"; }
  virtual inline const std::string GetConnectionName() const override { return "coinbase"; }

private:
  virtual void request_ticker() override {
      const std::string message = "{\"type\": \"subscribe\",\"product_ids\": [\"BTC-USD\", \"USDT-USD\"],\"channels\": [{\"name\": \"ticker\",\"product_ids\": [\"BTC-USD\"]}]}";
      WebsocketClient::send(message);
  }

  virtual std::optional<RawTicker> extract_ticker(client::message_ptr msg) override {
      // TODO: probably we need to just maintain a l2 orderbook to have current best bid and ask asap
      //std::cout << msg->get_payload() << std::endl;
      auto msg_json = json::parse(msg->get_payload());
      if (!msg_json.is_object() || msg_json["type"].get<std::string>() != "ticker") {
        std::cout << "Coinbase: Not a ticker" << std::endl;
        return {};
      }
      // TODO: this should be an order book ticker
      RawTicker ticker;
      ticker.bid = msg_json["best_bid"];
      ticker.bid_vol = "";
      ticker.ask = msg_json["best_ask"];
      ticker.ask_vol = "";
      // TODO: time provided in datetime format, needs parsing https://en.cppreference.com/w/cpp/io/manip/get_time
      ticker.source_ts = 0;
      ticker.exchange = GetConnectionName();
      ticker.symbol = msg_json["product_id"];
      return std::make_optional(ticker);
  }
};