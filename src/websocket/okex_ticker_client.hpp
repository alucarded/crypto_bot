#include "serialization_utils.hpp"
#include "websocket/ticker_client.hpp"

#include "json/json.hpp"

#include <string>

using json = nlohmann::json;

class OkexTickerClient : public TickerClient {
public:
  OkexTickerClient(Consumer<RawTicker>* ticker_consumer) : TickerClient(ticker_consumer) {
  }

virtual inline const std::string GetUrl() const override { return "wss://real.okex.com:8443/ws/v3"; }

virtual inline const std::string GetExchangeName() const override { return "okex"; }

private:
  virtual void request_ticker() override {
      const std::string message = "{\"op\": \"subscribe\", \"args\": [\"spot/ticker:BTC-USDT\"]}";
      TickerClient::send(message);
  }

  // TODO: maybe try implementing something like https://www.okex.com/docs/en/#spot_ws-limit
  virtual std::optional<RawTicker> extract_ticker(client::message_ptr msg) override {
      std::string decompressed_msg = utils::zlib_inflate(msg->get_payload());
      std::cout << decompressed_msg << std::endl;
      //auto msg_json = json::parse(decompressed_msg);
      // if (!msg_json.is_object()) {
      //   // std::cout << msg->get_payload() << std::endl;
      //   return {};
      // }
      // auto data = msg_json[1];
      // RawTicker ticker;
      // ticker.m_bid = data["b"][0];
      // ticker.m_bid_vol = data["b"][2];
      // ticker.m_ask = data["a"][0];
      // ticker.m_ask_vol = data["a"][2];
      // ticker.m_source_ts = 0; // not provided
      // ticker.m_exchange = GetExchangeName();
      // return std::make_optional(ticker);
      return {};
  }
};