#include "tickers_watcher.hpp"

#include "exchange/exchange_listener.h"
#include "serialization_utils.hpp"
#include "websocket/websocket_client.hpp"
#include "utils/spinlock.hpp"

#include "json/json.hpp"

#include <chrono>
#include <optional>
#include <string>
#include <thread>

using namespace std::chrono;

using json = nlohmann::json;

// TODO: rename to eg. BinanceBookTickerStream
class BinanceWebsocketClient : public WebsocketClient {
public:
  inline static const std::string NAME = "binance";

  BinanceWebsocketClient(ExchangeListener* exchange_listener)
      : WebsocketClient("wss://stream.binance.com:9443/ws/bookTicker", NAME), m_exchange_listener(exchange_listener), m_tickers_watcher(30000, NAME, this) {
        m_tickers_watcher.Start();
  }

  void SubscribeTicker(const std::string& symbol) {
      const std::string message = "{\"method\": \"SUBSCRIBE\",\"params\": [\"" + symbol + "@bookTicker\"],\"id\": " + std::to_string(++s_sub_id) +"}";
      m_subscription_msg.push_back(message);
      BOOST_LOG_TRIVIAL(info) << message;
      WebsocketClient::send(message);
  }

  void ListSubscriptions() {
      const std::string message = "{\"method\": \"LIST_SUBSCRIPTIONS\",\"id\": " + std::to_string(++s_sub_id) +"}";
      WebsocketClient::send(message);
  }

private:
  virtual void OnOpen(websocketpp::connection_hdl conn) override {
    for (const auto& msg : m_subscription_msg) {
      std::this_thread::sleep_for(500ms);
      WebsocketClient::send(msg);
    }
    m_exchange_listener->OnConnectionOpen(NAME);
  }

  virtual void OnClose(websocketpp::connection_hdl conn) override {
    m_exchange_listener->OnConnectionClose(NAME);
  }

  virtual void OnMessage(websocketpp::connection_hdl, client::message_ptr msg) override {
      auto msg_json = json::parse(msg->get_payload());
      BOOST_LOG_TRIVIAL(trace) << "Binance websocket message: " << msg_json;
      // TODO: Perhaps do not do schema validation for every message in production
      if (!msg_json.is_object()
          || !utils::check_message(msg_json, {"u", "s", "b", "B", "a", "A"})) {
        std::cout << "Binance: Not an expected ticker object: " << msg_json << std::endl;
        return;
      }
      Ticker ticker;
      ticker.m_bid = std::stod(msg_json["b"].get<std::string>());
      ticker.m_bid_vol = std::optional<double>(std::stod(msg_json["B"].get<std::string>()));
      ticker.m_ask = std::stod(msg_json["a"].get<std::string>());
      ticker.m_ask_vol = std::optional<double>(std::stod(msg_json["A"].get<std::string>()));
      // Transaction time (received in ms)
      ticker.m_source_ts = std::nullopt;//std::optional<int64_t>(msg_json["T"].get<int64_t>() * 1000);
      // TODO: perhaps generate timestamp in base class and pass it to this method
      auto now = system_clock::now();
      system_clock::duration tp = now.time_since_epoch();
      microseconds us = duration_cast<microseconds>(tp);
      ticker.m_arrived_ts = us.count();
      ticker.m_exchange = NAME;
      ticker.m_symbol = SymbolPair::FromBinanceString(msg_json["s"].get<std::string>());
      // TODO: should be unique across exchange
      ticker.m_id = 1;
      // TODO: reconnect/resubscribe all websocket clients when they are inactive for too long
      m_tickers_watcher.Set(SymbolPair(ticker.m_symbol), ticker.m_arrived_ts, ticker.m_arrived_ts);
      m_exchange_listener->OnTicker(ticker);
  }

private:
  ExchangeListener* m_exchange_listener;
  std::vector<std::string> m_subscription_msg;
  static int s_sub_id;
  TickersWatcher m_tickers_watcher;
};

int BinanceWebsocketClient::s_sub_id = 0;