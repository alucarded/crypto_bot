#include "serialization_utils.hpp"
#include "websocket/ticker_client.hpp"

#include "json/json.hpp"

#include <string>

using json = nlohmann::json;

class BybitTickerClient : public TickerClient {
public:
  BybitTickerClient(TickerConsumer* ticker_consumer) : TickerClient(ticker_consumer) {
    TickerClient::start("wss://stream.bybit.com/realtime");
  }

  virtual inline const std::string GetExchangeName() const override { return "bybit"; }

private:
  virtual void on_open(websocketpp::connection_hdl) override {
      std::cout << "Connection opened" << std::endl;
        const std::string message = "{\"op\": \"subscribe\", \"args\": [\"orderBookL2_25.BTCUSD\"]}";
        TickerClient::send(message);
  }

  virtual std::optional<RawTicker> extract_ticker(client::message_ptr msg) override {
    // TODO: implement order book with updates
      std::cout << msg->get_payload() << std::endl;
      return {};
  }
};