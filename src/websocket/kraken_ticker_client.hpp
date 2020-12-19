#include "serialization_utils.hpp"
#include "websocket/ticker_client.hpp"

#include "json/json.hpp"

#include <string>

using json = nlohmann::json;

class KrakenTickerClient : public TickerClient {
public:
  KrakenTickerClient(TickerConsumer* ticker_consumer) : TickerClient(ticker_consumer) {
    TickerClient::start("wss://ws.kraken.com");
  }

  virtual inline const std::string GetExchangeName() const override { return "kraken"; }

private:
  virtual void on_open(websocketpp::connection_hdl) override {
      std::cout << "Connection opened" << std::endl;
        const std::string message = "{\"event\": \"subscribe\",\"pair\": [\"XBT/USD\"],\"subscription\": {\"name\": \"ticker\"}}";
        TickerClient::send(message);
  }

  virtual std::optional<RawTicker> extract_ticker(client::message_ptr msg) override {
      auto msg_json = json::parse(msg->get_payload());
      if (nlohmann::json::value_t::array != msg_json.type()) {
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