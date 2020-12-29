#include "serialization_utils.hpp"
#include "websocket/ticker_client.hpp"

#include "json/json.hpp"

#include <string>

using json = nlohmann::json;

class PoloniexTickerClient : public TickerClient {
public:
  PoloniexTickerClient(Consumer<RawTicker>* ticker_consumer) : TickerClient(ticker_consumer) {
  }

  virtual inline const std::string GetUrl() const override { return "wss://api2.poloniex.com"; }

  virtual inline const std::string GetExchangeName() const override { return "poloniex"; }

private:
  virtual void request_ticker() override {
      const std::string message = "{\"command\": \"subscribe\", \"channel\": 1002}";
      TickerClient::send(message);
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
      ticker.m_bid = data[3];
      ticker.m_bid_vol = "";
      ticker.m_ask = data[2];
      ticker.m_ask_vol = "";
      ticker.m_source_ts = 0; // not provided
      ticker.m_exchange = GetExchangeName();
      return std::make_optional(ticker);
  }
};