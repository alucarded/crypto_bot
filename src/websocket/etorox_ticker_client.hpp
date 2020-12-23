#include "ticker_client.hpp"
#include "ticker_consumer.h"

#include "../json/json.hpp"

#include <string>
#include <list>

using json = nlohmann::json;

class EtoroxTickerClient : public TickerClient {
public:

  EtoroxTickerClient(TickerConsumer* ticker_consumer) : TickerClient(ticker_consumer) {
  }

  virtual inline const std::string GetUrl() const override { return ""; }
  virtual inline const std::string GetExchangeName() const override { return "etorox"; }

private:
  virtual void on_open(websocketpp::connection_hdl hdl) override {
      std::cout << "Connection opened" << std::endl;
        // const std::string message = "{\"event\": \"bts:subscribe\",\"data\": {\"channel\": \"order_book_btcusd\"}}";
        // TickerClient::send(message);
  }

  virtual void on_message(websocketpp::connection_hdl hdl, client::message_ptr msg) override {
      std::cout << "Got message" << std::endl;
      std::cout << msg->get_payload() << std::endl;

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
};