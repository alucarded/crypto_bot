#include "exchange/user_data_listener.hpp"
#include "http/kraken_client.hpp"
#include "model/kraken.h"
#include "model/symbol.h"
#include "websocket_client.hpp"

#include <boost/log/trivial.hpp>

#include <chrono>
#include <string>

using namespace std::chrono;

class KrakenUserDataStream : public WebsocketClient {
public:
  KrakenUserDataStream(KrakenClient* kraken_client, UserDataListener* user_data_listener)
      : WebsocketClient("wss://ws-auth.kraken.com", "kraken_user_data"),
        m_listen_key(kraken_client->GetWebSocketsToken()),
        m_kraken_client(kraken_client),
        m_user_data_listener(user_data_listener) {
  }

private:

  void SubscribeOwnTrades() {
    const std::string message = "{\"event\": \"subscribe\",\"subscription\": {\"name\": \"ownTrades\",\"token\":\"" + m_kraken_client->GetWebSocketsToken() +"\"}}";
    WebsocketClient::send(message);
  }

  void SubscribeOpenOrders() {
    const std::string message = "{\"event\": \"subscribe\",\"subscription\": {\"name\": \"openOrders\",\"token\":\"" + m_kraken_client->GetWebSocketsToken() +"\"}}";
    WebsocketClient::send(message);
  }

  virtual void OnOpen(websocketpp::connection_hdl conn) override {
    m_user_data_listener->OnConnectionOpen("kraken");

    SubscribeOwnTrades();
    SubscribeOpenOrders();
  }

  virtual void OnClose(websocketpp::connection_hdl conn) override {
    m_user_data_listener->OnConnectionClose("kraken");

    m_listen_key = m_kraken_client->GetWebSocketsToken();
  }

  virtual bool OnPing(websocketpp::connection_hdl conn, std::string payload) override {
  }

  virtual void OnMessage(websocketpp::connection_hdl, client::message_ptr msg) override {
    auto msg_json = json::parse(msg->get_payload());
    BOOST_LOG_TRIVIAL(trace) << msg_json;
    if (!msg_json.is_array()) {
      BOOST_LOG_TRIVIAL(trace) << msg_json << std::endl;
      return;
    }
    BOOST_LOG_TRIVIAL(debug) << msg_json << std::endl;
    auto channel = msg_json[msg_json.size()-2];
    if (channel == "openOrders") {
      for (const auto& obj : msg_json[0].items()) {
        auto order_json = obj.value();
        for (auto it = order_json.begin(); it != order_json.end(); ++it) {
          std::string oid = it.key();
          json order_val = it.value();
          auto order_builder = Order::CreateBuilder();
          order_builder.Id(oid);
          if (order_val.contains("descr")) {
            auto descr = order_val["descr"];
            order_builder.Symbol(SymbolPair::FromKrakenString(descr["pair"].get<std::string>()));
            order_builder.Side_(descr["type"].get<std::string>() == "buy" ? Side::BUY : Side::SELL);
            order_builder.Price(std::stod(descr["price"].get<std::string>()));
            order_builder.OrderType_(Order::GetType(descr["ordertype"].get<std::string>()));
          }
          if (order_val.contains("vol")) {
            order_builder.Quantity(std::stod(order_val["vol"].get<std::string>()));
          }
          if (order_val.contains("status")) {
            const auto& status_str = order_val["status"].get<std::string>();
            OrderStatus status = OrderStatus::UNKNOWN;
            if (status_str == "open" || status_str == "pending") {
              status = OrderStatus::NEW;
            } else if (status_str == "closed") {
              status = OrderStatus::FILLED;
            } else if (status_str == "canceled") {
              status = OrderStatus::CANCELED;
            } else if (status_str == "expired") {
              status = OrderStatus::EXPIRED;
            }
            order_builder.OrderStatus_(status);
          }
          Order order = order_builder.Build();
          order.SetExecutedQuantity(std::stod(order_val["vol_exec"].get<std::string>()));
          order.SetTotalCost(std::stod(order_val["cost"].get<std::string>()));
          m_user_data_listener->OnOrderUpdate(order);
        }
      }
    } else {
      BOOST_LOG_TRIVIAL(debug) << "Unsupported channel event: " << channel;
    }
  }

private:
  std::string m_listen_key;
  KrakenClient* m_kraken_client;
  UserDataListener* m_user_data_listener;
};