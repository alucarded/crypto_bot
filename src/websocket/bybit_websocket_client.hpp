#include "serialization_utils.hpp"
#include "websocket/websocket_client.hpp"

#include "json/json.hpp"

#include <string>

using json = nlohmann::json;

class BybitWebsocketClient : public WebsocketClient {
public:
  BybitWebsocketClient(Consumer<RawTicker>* ticker_consumer) : WebsocketClient(ticker_consumer) {
  }

  virtual inline const std::string GetUrl() const override { return "wss://stream.bybit.com/realtime"; }
  virtual inline const std::string GetConnectionName() const override { return "bybit"; }

private:
  virtual void request_ticker() override {
      const std::string message = "{\"op\": \"subscribe\", \"args\": [\"orderBookL2_25.BTCUSD\"]}";
      WebsocketClient::send(message);
  }

  virtual std::optional<RawTicker> extract_ticker(client::message_ptr msg) override {
    // TODO: implement order book with updates
      std::cout << msg->get_payload() << std::endl;
      return {};
  }
};