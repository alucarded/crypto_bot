#include "serialization_utils.hpp"
#include "websocket/ticker_client.hpp"

#include "json/json.hpp"

#include <string>

using json = nlohmann::json;

class BitstampTickerClient : public TickerClient {
public:
  BitstampTickerClient(TickerConsumer* ticker_consumer) : TickerClient(ticker_consumer) {
    TickerClient::start("wss://ws.bitstamp.net");
  }

  virtual inline const std::string GetExchangeName() const override { return "bitstamp"; }

private:
  virtual void on_open(websocketpp::connection_hdl) override {
      std::cout << "Connection opened" << std::endl;
        const std::string message = "{\"event\": \"bts:subscribe\",\"data\": {\"channel\": \"order_book_btcusd\"}}";
        TickerClient::send(message);
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
      ticker.m_exchange = GetExchangeName();
      return std::make_optional(ticker);
  }
};