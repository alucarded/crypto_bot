#include "http/binance_client.hpp"
#include "websocket_client.hpp"

#include <boost/log/trivial.hpp>

#include <string>

class BinanceUserDataStream : public WebsocketClient {
public:
  static BinanceUserDataStream Create(BinanceClient* binance_client) {
    auto res = binance_client->StartUserDataStream();
    if (!res.has_value()) {
      BOOST_LOG_TRIVIAL(error) << "Error getting user data stream listen key";
    }
    return BinanceUserDataStream(res.value(), binance_client);
  }

protected:
  BinanceUserDataStream(const std::string& listen_key, BinanceClient* binance_client)
      : WebsocketClient("wss://stream.binance.com:9443/ws/" + listen_key, "binance_user_data"), m_binance_client(binance_client) {
  }

private:
  virtual void OnOpen(websocketpp::connection_hdl conn) override {

  }

  virtual void OnClose(websocketpp::connection_hdl conn) override {

  }

  virtual void OnMessage(websocketpp::connection_hdl, client::message_ptr msg) override {
    auto msg_json = json::parse(msg->get_payload());
    BOOST_LOG_TRIVIAL(info) << msg_json << std::endl;
  }
private:
  BinanceClient* m_binance_client;
};