#include "exchange/user_data_listener.hpp"
#include "http/binance_client.hpp"
#include "model/binance.h"
#include "model/symbol.h"
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
    if (!msg_json.contains("e")) {
      BOOST_LOG_TRIVIAL(debug) << "Not an interesting event: " << msg_json;
      return;
    }
    const auto& event_type = msg_json["e"].get<std::string>();
    auto event_ms = msg_json["E"].get<uint64_t>();
    using namespace std::chrono;
    auto now_ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    BOOST_LOG_TRIVIAL(info) << "Event " + event_type + " happened at " + std::to_string(event_ms) + ", received at " + std::to_string(now_ms); 
    if (event_type == "outboundAccountPosition") {
      std::unordered_map<SymbolId, double> balances;
      for (const json& b : msg_json["B"]) {
        balances.insert({ BINANCE_ASSET_MAP.at(b["a"].get<std::string>()), std::stod(b["f"].get<std::string>()) });
        // b["l"] - locked amount
      }
      m_user_data_listener->OnAccountBalanceUpdate(AccountBalance(std::move(balances)));
    } else if (event_type == "balanceUpdate") {
      BOOST_LOG_TRIVIAL(warning) << "User data stream event " + event_type + " is not supported";
    } else if (event_type == "executionReport") {
      HandleExecutionReport(msg_json);
    } else if (event_type == "listStatus") {
      BOOST_LOG_TRIVIAL(warning) << "User data stream event " + event_type + " is not supported";
    } else {
      BOOST_LOG_TRIVIAL(error) << "Unexpected user data stream event: " + event_type;
    }
  }

private:

  void HandleExecutionReport(const json& msg_json) {
    Order order = Order::CreateBuilder()
      .Id(std::to_string(msg_json["i"].get<uint64_t>()))
      .ClientId(msg_json["c"].get<std::string>())
      .Symbol(SymbolPair(msg_json["s"].get<std::string>())())
      .Side_(msg_json["S"].get<std::string>() == "BUY" ? Side::BUY : Side::SELL)
      .OrderType_(Order::GetType(msg_json["o"].get<std::string>()))
      .Quantity(std::stod(msg_json["q"].get<std::string>()))
      .Price(std::stod(msg_json["p"].get<std::string>()))
      .ExecutionType_(Order::GetExecutionType(msg_json["x"].get<std::string>()))
      .OrderStatus_(Order::GetStatus(msg_json["X"].get<std::string>()))
      // TODO: more ?
      .Build();
    m_user_data_listener->OnOrderUpdate(order);
  }

private:
  std::string m_listen_key;
  BinanceClient* m_binance_client;
  UserDataListener* m_user_data_listener;
  uint64_t m_last_keepalive;
  uint64_t m_keepalive_interval;
};