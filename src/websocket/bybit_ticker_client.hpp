#include "serialization_utils.hpp"
#include "websocket/ticker_client.hpp"

#include "json/json.hpp"

#include <string>

using json = nlohmann::json;

class BybitTickerClient : public TickerClient {
public:
  BybitTickerClient(Consumer<RawTicker>* ticker_consumer) : TickerClient(ticker_consumer) {
  }

  virtual inline const std::string GetUrl() const override { return "wss://stream.bybit.com/realtime"; }
  virtual inline const std::string GetExchangeName() const override { return "bybit"; }

private:
  virtual void request_ticker() override {
      const std::string message = "{\"op\": \"subscribe\", \"args\": [\"orderBookL2_25.BTCUSD\"]}";
      TickerClient::send(message);
  }

  virtual std::optional<RawTicker> extract_ticker(client::message_ptr msg) override {
    // TODO: implement order book with updates
      std::cout << msg->get_payload() << std::endl;
      return {};
  }
};