#include "serialization_utils.hpp"
#include "websocket/websocket_client.hpp"

#include "json/json.hpp"

#include <string>

using json = nlohmann::json;

// TODO: connection is disconnected after 24 hours - need to re-connect
class HuobiGlobalWebsocketClient : public WebsocketClient {
public:
  HuobiGlobalWebsocketClient(Consumer<RawTicker>* ticker_consumer) : WebsocketClient(ticker_consumer) {
  }

  virtual inline const std::string GetUrl() const override { return "wss://api.btcgateway.pro/swap-ws"; }
  virtual inline const std::string GetConnectionName() const override { return "huobiglobal"; }

private:
  inline static const std::string CHANNEL = "market.btcusdt.depth.step0";

  virtual void request_ticker() override {
      const std::string message = "{\"sub\": \"" + CHANNEL + "\",\"id\": \"1234\"}";
      WebsocketClient::send(message);
  }

  virtual std::optional<RawTicker> extract_ticker(client::message_ptr msg) override {
      std::string decompressed_msg = utils::gzip_decompress(msg->get_payload());
      auto msg_json = json::parse(decompressed_msg);
      // TODO: Perhaps do not do schema validation for every message in production
      if (!msg_json.is_object()) {
        std::cout << "Huobi Global: Not an object" << std::endl;
        return {};
      }
      if (msg_json.contains("ping")) {
        //std::cout << "Huobi Global: Ping" << std::endl;
        websocketpp::lib::error_code ec;
        //     void pong(connection_hdl hdl, std::string const & payload,
        // lib::error_code & ec);
        //{ "op":"pong", "ts":1492420473027 }
        //m_endpoint.pong(m_con->get_handle(), "{\"pong\": " + std::to_string(msg_json["ping"].get<uint64_t>()) +"}", ec);
        //m_endpoint.pong(m_con->get_handle(), "{\"op\":\"pong\", \"ts\":" + std::to_string(msg_json["ping"].get<uint64_t>()) +"}", ec);
        //std::string message = "{\"op\":\"pong\", \"ts\":" + std::to_string(msg_json["ping"].get<uint64_t>()) +"}";
        std::string message = "{\"pong\": " + std::to_string(msg_json["ping"].get<uint64_t>()) +"}";
        WebsocketClient::send(message);
        return {};
      }
      if (!utils::check_message(msg_json, {"ch", "ts", "tick"})
          || msg_json["ch"].get<std::string>() != CHANNEL) {
        std::cout << "Huobi Global: Not an expected ticker object" << std::endl;
        std::cout << decompressed_msg << std::endl;
        return {};
      }
      auto data = msg_json["tick"];
      RawTicker ticker;
      ticker.bid = std::to_string(data["bids"][0][0].get<double>());
      ticker.bid_vol = std::to_string(data["bids"][0][1].get<int>());
      ticker.ask = std::to_string(data["asks"][0][0].get<double>());
      ticker.ask_vol = std::to_string(data["asks"][0][1].get<int>());
      ticker.source_ts = data["ts"].get<uint64_t>() * 1000; // to microseconds
      ticker.exchange = GetConnectionName();
      ticker.symbol = "BTC-USDT";
      return std::make_optional(ticker);
  }
};