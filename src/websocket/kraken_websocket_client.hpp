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
#include <thread>

using json = nlohmann::json;

namespace {

static int quick_pow10(int n)
{
    static int pow10[10] = {
        1, 10, 100, 1000, 10000, 
        100000, 1000000, 10000000, 100000000, 1000000000
    };

    return pow10[n]; 
}

}

class KrakenWebsocketClient : public WebsocketClient {
public:
  inline static const std::string NAME = "kraken";
  // TODO: this should be per symbol
  inline static const size_t PRICE_SENT_PRECISION = 5;
  inline static const size_t VOLUME_SENT_PRECISION = 8;
  inline static const size_t TIMESTAMP_SENT_PRECISION = 6;

  KrakenWebsocketClient(ExchangeListener* exchange_listener) : WebsocketClient("wss://beta-ws.kraken.com", NAME), m_exchange_listener(exchange_listener) {
  }

  void SubscribeTicker(const std::string& symbol) {
    const std::string message = "{\"event\": \"subscribe\",\"pair\": [\"" + symbol + "\"],\"subscription\": {\"name\": \"ticker\"}}";
    m_subscription_msg.push_back(message);
    WebsocketClient::send(message);
  }

  void SubscribeOrderBook(const std::string& symbol) {
    std::string message = OrderBookSubscribeMsg(symbol);
    m_subscription_msg.push_back(message);
    WebsocketClient::send(message);
  }

  void UnsubscribeOrderBook(const std::string& symbol) {
    const std::string message = "{\"event\": \"unsubscribe\",\"pair\": [\"" + symbol + "\"],\"subscription\": {\"name\": \"book\",\"depth\": 10}}";
    WebsocketClient::send(message);
  }

private:

  std::string OrderBookSubscribeMsg(const std::string& symbol, bool subscribe = true) {
    return std::string("{\"event\": \"") + (subscribe ? "subscribe" : "unsubscribe") + "\",\"pair\": [\"" + symbol + "\"],\"subscription\": {\"name\": \"book\",\"depth\": 10}}";
  }

  virtual void OnOpen(websocketpp::connection_hdl conn) override {
    for (const auto& msg : m_subscription_msg) {
      WebsocketClient::send(msg);
    }
    m_exchange_listener->OnConnectionOpen(NAME);
  }

  virtual void OnClose(websocketpp::connection_hdl conn) override {
    m_exchange_listener->OnConnectionClose(NAME);
  }

  virtual void OnMessage(websocketpp::connection_hdl, client::message_ptr msg) override {
    auto msg_json = json::parse(msg->get_payload());
    std::cout << msg_json << std::endl;
    if (!msg_json.is_array()) {
      // It is most probably a publication or response
      // TODO:
      return;
    }
    auto channel_name = msg_json[2];
    if (channel_name == "ticker") {
      m_exchange_listener->OnTicker(RawTicker::ToTicker(ParseTicker(msg_json[1])));
    } else if (channel_name.get<std::string>().find("book") != std::string::npos) {
      bool is_valid = false;
      auto book_obj = msg_json[1];
      if (book_obj.contains("as")) {
        is_valid = true;
        OnOrderBookSnapshot(book_obj);
      } else if (book_obj.contains("a") || book_obj.contains("b")) {
        is_valid = OnOrderBookUpdate(book_obj);
      } else {
        BOOST_LOG_TRIVIAL(error) << "Unexpected book message!";
      }
      if (is_valid) {
        m_exchange_listener->OnOrderBookUpdate(m_order_book);
      } else {
        m_order_book.clear();
        // TODO: implement state machine, with subscription class objects in a vector as a state,
        // with which subscriptions can be re-created upon receiving unsubscribed publication
        WebsocketClient::send(OrderBookSubscribeMsg("XBT/USDT"));
      }
    }
  }

  RawTicker ParseTicker(json data) {
      RawTicker ticker;
      ticker.m_bid = data["b"][0];
      ticker.m_bid_vol = data["b"][2];
      ticker.m_ask = data["a"][0];
      ticker.m_ask_vol = data["a"][2];
      ticker.m_source_ts = 0; // not provided
      // TODO: perhaps generate timestamp in base class and pass it to this method
      using namespace std::chrono;
      auto now = system_clock::now();
      system_clock::duration tp = now.time_since_epoch();
      microseconds us = duration_cast<microseconds>(tp);
      ticker.m_arrived_ts = us.count();
      ticker.m_exchange = NAME;
      // TODO: use enum
      ticker.m_symbol = "BTCUSDT";
      return ticker;
  }

  void OnOrderBookSnapshot(const json& snapshot_obj) {
    for (const json& lvl : snapshot_obj["as"]) {
      // Upsert price level
      PriceLevel pl = ParsePriceLevel(lvl);
      m_order_book.UpsertAsk(pl);
    }
    for (const json& lvl : snapshot_obj["bs"]) {
      // Upsert price level
      PriceLevel pl = ParsePriceLevel(lvl);
      m_order_book.UpsertBid(pl);
    }
  }

  bool OnOrderBookUpdate(const json& update_obj) {
    if (update_obj.contains("a")) {
      for (const json& lvl : update_obj["a"]) {
        // Upsert price level
        PriceLevel pl = ParsePriceLevel(lvl);
        if (pl.GetVolume() == 0.0) {
          m_order_book.DeleteAsk(pl.GetPrice());
        } else {
          m_order_book.UpsertAsk(pl);
        }
      }
    }
    if (update_obj.contains("b")) {
      for (const json& lvl : update_obj["b"]) {
        // Upsert price level
        PriceLevel pl = ParsePriceLevel(lvl);
        if (pl.GetVolume() == 0.0) {
          m_order_book.DeleteBid(pl.GetPrice());
        } else {
          m_order_book.UpsertBid(pl);
        }
      }
    }
    std::cout << m_order_book << std::endl;
    if (update_obj.contains("c")) {
      std::string crc32_in = CalculateChecksumInput();
      std::cout << crc32_in << std::endl;
      boost::crc_32_type crc;
      crc.process_bytes(crc32_in.c_str(), crc32_in.size());
      if (crc.checksum() != std::stoul(update_obj["c"].get<std::string>())) {
        BOOST_LOG_TRIVIAL(warning) << "Wrong checksum!";
        return false;
      }
    }
    return true;
  }

  PriceLevel ParsePriceLevel(const json& lvl) {
    // Parse price
    auto p = lvl[0].get<std::string>();
    size_t dot_pos = p.find('.');
    if (p.size() - 1 - dot_pos != PRICE_SENT_PRECISION) {
      throw std::runtime_error("Unexpected price precision");
    }
    p.erase(dot_pos, 1);

    // Parse timestamp
    auto ts = lvl[2].get<std::string>();
    dot_pos = ts.find('.');
    if (ts.size() - 1 - dot_pos != TIMESTAMP_SENT_PRECISION) {
      throw std::runtime_error("Unexpected timestamp precision");
    }
    ts.erase(dot_pos, 1);

    return PriceLevel(price_t(std::stoull(p)), std::stod(lvl[1].get<std::string>()), std::stoull(ts));
  }

  std::string CalculateChecksumInput() {
    std::vector<std::string> partial_input;
    const auto& asks = m_order_book.GetAsks(); 
    std::transform(asks.rbegin(), asks.rend(), std::back_inserter(partial_input),
        [](const PriceLevel& pl) -> std::string {
          return std::to_string(uint64_t(pl.GetPrice())) + std::to_string(uint64_t(pl.GetVolume()*quick_pow10(8)));
        });
    const auto& bids = m_order_book.GetBids(); 
    std::transform(bids.rbegin(), bids.rend(), std::back_inserter(partial_input),
        [](const PriceLevel& pl) -> std::string {
          return std::to_string(uint64_t(pl.GetPrice())) + std::to_string(uint64_t(pl.GetVolume()*quick_pow10(8)));
        });
    std::string res;
    for (const auto &piece : partial_input) res += piece;
    return res;
  }
private:
  ExchangeListener* m_exchange_listener;
  std::vector<std::string> m_subscription_msg;
  OrderBook m_order_book;
};