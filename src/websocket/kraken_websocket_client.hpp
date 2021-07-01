#include "kraken_order_book_handler.hpp"
#include "tickers_watcher.hpp"

#include "exchange/exchange_listener.h"
#include "serialization_utils.hpp"
#include "websocket/websocket_client.hpp"

#include <boost/crc.hpp>
#include <boost/log/trivial.hpp>
#include "json/json.hpp"

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <string>

using namespace std::chrono;

using json = nlohmann::json;

enum class KrakenOrderBookDepth : size_t {
  DEPTH_10 = 10,
  DEPTH_25 = 25,
  DEPTH_100 = 100,
  DEPTH_500 = 500,
  DEPTH_1000 = 1000
};

class KrakenWebsocketClient : public WebsocketClient {
public:
  inline static const std::string NAME = "kraken";

  static const std::unordered_map<SymbolPairId, PrecisionSettings> SENT_PRECISIONS;

  KrakenWebsocketClient(ExchangeListener* exchange_listener, bool validate = true)
      : WebsocketClient("wss://ws.kraken.com", NAME), m_exchange_listener(exchange_listener), m_tickers_watcher(30000, NAME, this), m_order_book_handler(validate) {
    m_tickers_watcher.Start();
  }

  void SubscribeTicker(const std::string& symbol) {
    const std::string message = "{\"event\": \"subscribe\",\"pair\": [\"" + symbol + "\"],\"subscription\": {\"name\": \"ticker\"}}";
    m_subscription_msg.push_back(message);
    WebsocketClient::send(message);
  }

  void SubscribeOrderBook(const std::string& symbol, KrakenOrderBookDepth requested_depth) {
    std::string message = OrderBookSubscribeMsg(symbol, requested_depth);
    m_subscription_msg.push_back(message);
    SymbolPair sp = SymbolPair::FromKrakenString(symbol);
    SymbolPairId spid = SymbolPairId(sp);
    m_order_books.emplace(spid, OrderBook(NAME, spid, static_cast<size_t>(requested_depth), SENT_PRECISIONS.at(spid)));
    WebsocketClient::send(message);
  }

  void SubscribeBookTicker(const std::string& symbol) {
    std::string message = OrderBookSubscribeMsg(symbol, KrakenOrderBookDepth::DEPTH_10);
    m_subscription_msg.push_back(message);
    SymbolPair sp = SymbolPair::FromKrakenString(symbol);
    SymbolPairId spid = SymbolPairId(sp);
    m_order_books.emplace(spid, OrderBook(NAME, spid, 1, SENT_PRECISIONS.at(spid)));
    WebsocketClient::send(message);
  }

  void UnsubscribeOrderBook(const std::string& symbol) {
    const std::string message = "{\"event\": \"unsubscribe\",\"pair\": [\"" + symbol + "\"],\"subscription\": {\"name\": \"book\",\"depth\": 10}}";
    WebsocketClient::send(message);
  }

private:

  std::string OrderBookSubscribeMsg(const std::string& symbol, KrakenOrderBookDepth depth, bool subscribe = true) {
    return std::string("{\"event\": \"") + (subscribe ? "subscribe" : "unsubscribe") + "\",\"pair\": [\"" + symbol + "\"],\"subscription\": {\"name\": \"book\",\"depth\": " + std::to_string(static_cast<size_t>(depth))  + "}}";
  }

  virtual void OnOpen(websocketpp::connection_hdl conn) override {
    for (const auto& msg : m_subscription_msg) {
      WebsocketClient::send(msg);
      std::this_thread::sleep_for(400ms);
    }
    m_exchange_listener->OnConnectionOpen(NAME);
  }

  virtual void OnClose(websocketpp::connection_hdl conn) override {
    m_exchange_listener->OnConnectionClose(NAME);
  }

  virtual void OnMessage(websocketpp::connection_hdl, client::message_ptr msg) override {
    auto msg_json = json::parse(msg->get_payload());
    BOOST_LOG_TRIVIAL(trace) << "Kraken websocket message: " << msg_json;
    if (!msg_json.is_array()) {
      // It is most probably a publication or response
      // TODO:
      return;
    }
    auto channel_name = msg_json[msg_json.size() - 2];
    const auto& symbol = msg_json[msg_json.size() - 1].get<std::string>();
    if (channel_name == "ticker") {
      m_exchange_listener->OnBookTicker(ParseTicker(msg_json[1], symbol));
    } else if (channel_name.get<std::string>().find("book") != std::string::npos) {
      SymbolPair symbol_pair = SymbolPair::FromKrakenString(symbol);
      SymbolPairId pair_id = SymbolPairId(symbol_pair);
      if (m_order_books.count(pair_id) < 1) {
        throw std::runtime_error("Order book update for an unexpected symbol pair");
      }
      OrderBook& ob = m_order_books.at(pair_id);
      bool is_valid = m_order_book_handler.OnOrderBookMessage(msg_json, ob);
      if (is_valid) {
        auto now = system_clock::now();
        system_clock::duration tp = now.time_since_epoch();
        microseconds us = duration_cast<microseconds>(tp);
        m_tickers_watcher.Set(ob.GetSymbolPairId(), us.count(), ob.GetLatestUpdateTimestamp());
        m_exchange_listener->OnOrderBookUpdate(ob);
      } else {
        // TODO: implement state machine, with subscription class objects in a vector as a state,
        // with which subscriptions can be re-created upon receiving unsubscribed publication
        WebsocketClient::send(OrderBookSubscribeMsg(symbol, static_cast<KrakenOrderBookDepth>(ob.GetDepth())));
      }
    }
  }

  Ticker ParseTicker(json data, const std::string& symbol) {
      Ticker ticker;
      ticker.bid = std::stod(data["b"][0].get<std::string>());
      ticker.bid_vol = std::optional<double>(std::stod(data["b"][2].get<std::string>()));
      ticker.ask = std::stod(data["a"][0].get<std::string>());
      ticker.ask_vol = std::optional<double>(std::stod(data["a"][2].get<std::string>()));
      ticker.source_ts = std::nullopt; // not provided
      // TODO: perhaps generate timestamp in base class and pass it to this method
      auto now = system_clock::now();
      system_clock::duration tp = now.time_since_epoch();
      microseconds us = duration_cast<microseconds>(tp);
      ticker.arrived_ts = us.count();
      ticker.exchange = NAME;
      static uint64_t last_ticker_id = 0;
      ticker.id = last_ticker_id++;
      ticker.symbol = SymbolPair::FromKrakenString(symbol);
      m_tickers_watcher.Set(SymbolPair(ticker.symbol), ticker.arrived_ts, ticker.arrived_ts);
      return ticker;
  }

private:
  ExchangeListener* m_exchange_listener;
  std::vector<std::string> m_subscription_msg;
  std::unordered_map<SymbolPairId, OrderBook> m_order_books;
  TickersWatcher m_tickers_watcher;
  KrakenOrderBookHandler m_order_book_handler;
};

const std::unordered_map<SymbolPairId, PrecisionSettings> KrakenWebsocketClient::SENT_PRECISIONS = {
  // price_precision, volume_precision, timestamp_precision
  {SymbolPairId::BTC_USDT, PrecisionSettings(5, 8, 6)},
  {SymbolPairId::DOT_BTC, PrecisionSettings(10, 8, 6)},
  {SymbolPairId::DOT_USDT, PrecisionSettings(6, 8, 6)},
  {SymbolPairId::EOS_BTC, PrecisionSettings(8, 8, 6)},
  {SymbolPairId::EOS_USDT, PrecisionSettings(6, 8, 6)},
  {SymbolPairId::ADA_BTC, PrecisionSettings(9, 8, 6)},
  {SymbolPairId::ADA_USDT, PrecisionSettings(8, 8, 6)},
  {SymbolPairId::ETH_BTC, PrecisionSettings(6, 8, 6)},
  {SymbolPairId::EOS_ETH, PrecisionSettings(8, 8, 6)},
  {SymbolPairId::ETH_USDT, PrecisionSettings(5, 8, 6)},
  {SymbolPairId::XLM_BTC, PrecisionSettings(9, 8, 6)},
};