#include "serialization_utils.hpp"
#include "websocket/websocket_client.hpp"

#include "json/json.hpp"

#include <string>

using json = nlohmann::json;

class KrakenWebsocketClient : public WebsocketClient {
public:
  KrakenWebsocketClient(Consumer<RawTicker>* ticker_consumer) : WebsocketClient(ticker_consumer) {
  }



  virtual inline const std::string GetUrl() const override { return "wss://beta-ws.kraken.com"; }

  virtual inline const std::string GetExchangeName() const override { return "kraken"; }

private:
  virtual void request_ticker() override {
      const std::string message = "{\"event\": \"subscribe\",\"pair\": [\"XBT/USDT\"],\"subscription\": {\"name\": \"ticker\"}}";
      WebsocketClient::send(message);
  }

  virtual std::optional<RawTicker> extract_ticker(client::message_ptr msg) override {
      auto msg_json = json::parse(msg->get_payload());
      //std::cout << msg_json << std::endl;
      if (!msg_json.is_array()) {
        return {};
      }
      auto data = msg_json[1];
      RawTicker ticker;
      ticker.m_bid = data["b"][0];
      ticker.m_bid_vol = data["b"][2];
      ticker.m_ask = data["a"][0];
      ticker.m_ask_vol = data["a"][2];
      ticker.m_source_ts = 0; // not provided
      // TODO: perhaps generate timestamp in base class and pass it to this method
      using namespace std::chrono;
      auto now = system_clock::now();
      system_clock::duration tp = now.time_since_epoch();
      microseconds us = duration_cast<microseconds>(tp);
      ticker.m_arrived_ts = us.count();
      ticker.m_exchange = GetExchangeName();
      // TODO: use enum
      ticker.m_symbol = "BTCUSDT";
      return std::make_optional(ticker);
  }
};