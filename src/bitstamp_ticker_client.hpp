#include "ticker_client.hpp"
#include "ticker_consumer.h"

#include "../json/json.hpp"

#include <string>
#include <list>

using json = nlohmann::json;

class BitstampTickerClient : public TickerClient {
public:
  BitstampTickerClient() {
    TickerClient::start("wss://ws.bitstamp.net");
  }

  BitstampTickerClient(TickerConsumer* ticker_consumer) : BitstampTickerClient() {
    this->ticker_consumer = ticker_consumer;
  }

  virtual inline const std::string GetExchangeName() const override { return "Bitstamp"; }

private:
  virtual void on_open(websocketpp::connection_hdl hdl) override {
      std::cout << "Connection opened" << std::endl;
        const std::string message = "{\"event\": \"bts:subscribe\",\"data\": {\"channel\": \"order_book_btcusd\"}}";
        TickerClient::send(message);
  }

  virtual void on_message(websocketpp::connection_hdl hdl, client::message_ptr msg) override {
      //std::cout << "Got message" << std::endl;
      //std::cout << msg->get_payload() << std::endl;
      // TODO: create Ticker structure from the data
      auto msg_json = json::parse(msg->get_payload());
      auto data = msg_json["data"];
      if (!check_message(data, {"bids", "asks", "microtimestamp"})) {
        //std::cout << msg->get_payload() << std::endl;
        return;
      }
      RawTicker ticker;
      ticker.m_bid = data["bids"][0][0];
      ticker.m_bid_vol = data["bids"][0][1];
      ticker.m_ask = data["asks"][0][0];
      ticker.m_ask_vol = data["asks"][0][1];
      ticker.m_source_ts = std::stoull(data["microtimestamp"].get<std::string>());
      ticker.m_exchange = GetExchangeName();
      if (ticker_consumer) {
        ticker_consumer->Consume(ticker);
      }
  }

  bool check_message(json j, const std::string& field) {
      if (!j.contains(field)) {
        std::cout << "No " + field + ", skipping message" << std::endl;
        return false;
      }
      return true;
  }

  bool check_message(json j, const std::list<std::string>& fields) {
      for (const std::string& field : fields) {
        if (!check_message(j, field)) {
          return false;
        }
      }
      return true;
  }

  TickerConsumer* ticker_consumer;
};