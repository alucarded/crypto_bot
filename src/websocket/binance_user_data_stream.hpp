#include "exchange/user_data_listener.hpp"
#include "http/binance_client.hpp"
#include "websocket_client.hpp"

#include <boost/log/trivial.hpp>

#include <chrono>
#include <string>

using namespace std::chrono;

class BinanceUserDataStream : public WebsocketClient {
public:
  static BinanceUserDataStream Create(BinanceClient* binance_client, UserDataListener* user_data_listener) {
    auto res = binance_client->StartUserDataStream();
    if (!res.has_value()) {
      BOOST_LOG_TRIVIAL(error) << "Error getting user data stream listen key";
    }
    return BinanceUserDataStream(res.value(), binance_client, user_data_listener);
  }

protected:
  BinanceUserDataStream(const std::string& listen_key, BinanceClient* binance_client, UserDataListener* user_data_listener)
      : WebsocketClient("wss://stream.binance.com:9443/ws/" + listen_key, "binance_user_data"),
        m_listen_key(listen_key),
        m_binance_client(binance_client),
        m_user_data_listener(user_data_listener),
        m_last_keepalive(duration_cast<microseconds>(system_clock::now().time_since_epoch()).count()),
        m_keepalive_interval(1800 * 1000 * 1000) {
  }

private:
  virtual void OnOpen(websocketpp::connection_hdl conn) override {
    m_user_data_listener->OnConnectionOpen("binance");
  }

  virtual void OnClose(websocketpp::connection_hdl conn) override {
    m_user_data_listener->OnConnectionClose("binance");

    auto res = m_binance_client->StartUserDataStream();
    if (!res.has_value()) {
      BOOST_LOG_TRIVIAL(error) << "Error getting user data stream listen key";
    }
    m_uri = "wss://stream.binance.com:9443/ws/" + res.value();
  }

  virtual bool OnPing(websocketpp::connection_hdl conn, std::string payload) override {
    auto now_us = duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
    if (now_us - m_last_keepalive > m_keepalive_interval) {
      // TODO: do it asynchronously not to block the thread too much
      m_binance_client->KeepaliveUserDataStream(m_listen_key);
      m_last_keepalive = now_us;
    }
  }

  virtual void OnMessage(websocketpp::connection_hdl, client::message_ptr msg) override {
    auto msg_json = json::parse(msg->get_payload());
    BOOST_LOG_TRIVIAL(info) << msg_json << std::endl;
    // TODO: parse and pass to user data listener
  }
private:
  std::string m_listen_key;
  BinanceClient* m_binance_client;
  UserDataListener* m_user_data_listener;
  uint64_t m_last_keepalive;
  uint64_t m_keepalive_interval;
};