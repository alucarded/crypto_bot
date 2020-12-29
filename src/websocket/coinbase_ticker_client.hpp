#include "serialization_utils.hpp"
#include "websocket/ticker_client.hpp"

#include "json/json.hpp"

#include <string>

using json = nlohmann::json;

class CoinbaseTickerClient : public TickerClient {
public:
  CoinbaseTickerClient(TickerConsumer* ticker_consumer) : TickerClient(ticker_consumer) {
  }

  virtual inline const std::string GetUrl() const override { return "wss://ws-feed.pro.coinbase.com"; }
  virtual inline const std::string GetExchangeName() const override { return "coinbase"; }

private:
  virtual void request_ticker() override {
      const std::string message = "{\"type\": \"subscribe\",\"product_ids\": [\"BTC-USD\", \"USDT-USD\"],\"channels\": [{\"name\": \"ticker\",\"product_ids\": [\"BTC-USD\"]}]}";
      TickerClient::send(message);
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
      ticker.m_bid = msg_json["best_bid"];
      ticker.m_bid_vol = "";
      ticker.m_ask = msg_json["best_ask"];
      ticker.m_ask_vol = "";
      // TODO: time provided in datetime format, needs parsing https://en.cppreference.com/w/cpp/io/manip/get_time
      ticker.m_source_ts = 0;
      ticker.m_exchange = GetExchangeName();
      ticker.m_symbol = msg_json["product_id"];
      return std::make_optional(ticker);
  }
};