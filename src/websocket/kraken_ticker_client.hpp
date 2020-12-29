#include "serialization_utils.hpp"
#include "websocket/ticker_client.hpp"

#include "json/json.hpp"

#include <string>

using json = nlohmann::json;

class KrakenTickerClient : public TickerClient {
public:
  KrakenTickerClient(Consumer<RawTicker>* ticker_consumer) : TickerClient(ticker_consumer) {
  }

virtual inline const std::string GetUrl() const override { return "wss://ws.kraken.com"; }

virtual inline const std::string GetExchangeName() const override { return "kraken"; }

private:
  virtual void request_ticker() override {
      const std::string message = "{\"event\": \"subscribe\",\"pair\": [\"XBT/USDT\"],\"subscription\": {\"name\": \"ticker\"}}";
      TickerClient::send(message);
  }

  virtual std::optional<RawTicker> extract_ticker(client::message_ptr msg) override {
      auto msg_json = json::parse(msg->get_payload());
      //std::cout << msg->get_payload() << std::endl;
      if (!msg_json.is_array()) {
        // std::cout << msg->get_payload() << std::endl;
        return {};
      }
      auto data = msg_json[1];
      RawTicker ticker;
      ticker.m_bid = data["b"][0];
      ticker.m_bid_vol = data["b"][2];
      ticker.m_ask = data["a"][0];
      ticker.m_ask_vol = data["a"][2];
      ticker.m_source_ts = 0; // not provided
      ticker.m_exchange = GetExchangeName();
      return std::make_optional(ticker);
  }
};