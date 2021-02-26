#include "serialization_utils.hpp"
#include "websocket/websocket_client.hpp"

#include "json/json.hpp"

#include <string>

using json = nlohmann::json;

class FtxWebsocketClient : public WebsocketClient {
public:
  FtxWebsocketClient(Consumer<RawTicker>* ticker_consumer) : WebsocketClient(ticker_consumer) {
  }

  virtual inline const std::string GetUrl() const override { return "wss://ftx.com/ws/"; }
  virtual inline const std::string GetConnectionName() const override { return "ftx"; }

private:
  virtual void request_ticker() override {
      const std::string message = "{\"op\": \"subscribe\", \"channel\": \"ticker\", \"market\": \"BTC/USDT\"}";
      WebsocketClient::send(message);
  }

  // TODO: maybe try implementing something like https://www.okex.com/docs/en/#spot_ws-limit
  virtual std::optional<RawTicker> extract_ticker(client::message_ptr msg) override {
      //std::cout << msg->get_payload() << std::endl;
      auto msg_json = json::parse(msg->get_payload());
      if (!msg_json.is_object()
          || msg_json["channel"].get<std::string>() != "ticker") {
        std::cout << "Ftx: unexpected message" << std::endl;
        // std::cout << msg->get_payload() << std::endl;
        return {};
      }
      if (msg_json["type"].get<std::string>() != "update") {
        // std::cout << msg->get_payload() << std::endl;
        return {};
      }
      auto data = msg_json["data"];
      RawTicker ticker;
      ticker.m_bid = utils::to_string(data["bid"].get<double>(), 1);
      ticker.m_bid_vol = std::to_string(data["bidSize"].get<double>());
      ticker.m_ask = utils::to_string(data["ask"].get<double>(), 1);
      ticker.m_ask_vol = std::to_string(data["askSize"].get<double>());
      ticker.m_source_ts = static_cast<uint64_t>(data["time"].get<double>() * 1000000);
      ticker.m_exchange = GetConnectionName();
      return std::make_optional(ticker);
  }
};