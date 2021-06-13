#include "tickers_watcher.hpp"

#include "exchange/exchange_listener.h"
#include "model/trade_ticker.h"
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

class BinanceTradeTickerStream : public WebsocketClient {
public:
  inline static const std::string NAME = "binance";

  BinanceTradeTickerStream(ExchangeListener* exchange_listener)
      : WebsocketClient("wss://stream.binance.com:9443/ws/trade", NAME), m_exchange_listener(exchange_listener), m_tickers_watcher(30000, NAME, this) {
        m_tickers_watcher.Start();
  }

  void SubscribeTicker(const std::string& symbol) {
      const std::string message = "{\"method\": \"SUBSCRIBE\",\"params\": [\"" + symbol + "@trade\"],\"id\": " + std::to_string(++s_sub_id) +"}";
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
      BOOST_LOG_TRIVIAL(trace) << "Binance trade ticker: " << msg_json;
      // TODO: Perhaps do not do schema validation for every message in production
      if (!msg_json.is_object()
          || !utils::check_message(msg_json, {"e", "E", "s", "t", "p", "q", "T", "m"})) {
        BOOST_LOG_TRIVIAL(warning) << "Binance: Not an expected trade ticker object: " << msg_json << std::endl;
        return;
      }
      TradeTicker ticker;
      ticker.event_time = msg_json["E"].get<uint64_t>();
      ticker.trade_time = msg_json["T"].get<uint64_t>();
      ticker.symbol = SymbolPair::FromBinanceString(msg_json["s"].get<std::string>());
      ticker.trade_id = std::to_string(msg_json["t"].get<uint64_t>());
      ticker.price = std::stod(msg_json["p"].get<std::string>());
      ticker.qty = std::stod(msg_json["q"].get<std::string>());
      ticker.is_market_maker = msg_json["m"].get<bool>();

      // TODO: reconnect/resubscribe all websocket clients when they are inactive for too long
      // m_tickers_watcher.Set(SymbolPair(ticker.symbol), ticker.arrived_ts, ticker.arrived_ts);
      m_exchange_listener->OnTradeTicker(ticker);
  }

private:
  ExchangeListener* m_exchange_listener;
  std::vector<std::string> m_subscription_msg;
  static int s_sub_id;
  TickersWatcher m_tickers_watcher;
};

int BinanceTradeTickerStream::s_sub_id = 0;