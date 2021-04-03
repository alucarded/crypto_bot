#include "tickers_watcher.hpp"

#include "exchange/exchange_listener.h"
#include "serialization_utils.hpp"
#include "websocket/websocket_client.hpp"
#include "utils/math.hpp"
#include "utils/spinlock.hpp"

#include <boost/crc.hpp>
#include <boost/log/trivial.hpp>
#include "json/json.hpp"

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <string>

using namespace std::chrono;

using json = nlohmann::json;

class KrakenWebsocketClient : public WebsocketClient {
public:
  inline static const std::string NAME = "kraken";

  static const std::unordered_map<SymbolPairId, PrecisionSettings> SENT_PRECISIONS;

  KrakenWebsocketClient(ExchangeListener* exchange_listener) : WebsocketClient("wss://ws.kraken.com", NAME), m_exchange_listener(exchange_listener), m_tickers_watcher(NAME) {
    m_tickers_watcher.Start();
  }

  void SubscribeTicker(const std::string& symbol) {
    const std::string message = "{\"event\": \"subscribe\",\"pair\": [\"" + symbol + "\"],\"subscription\": {\"name\": \"ticker\"}}";
    m_subscription_msg.push_back(message);
    WebsocketClient::send(message);
  }

  void SubscribeOrderBook(const std::string& symbol) {
    std::string message = OrderBookSubscribeMsg(symbol);
    m_subscription_msg.push_back(message);
    SymbolPair sp{symbol};
    SymbolPairId spid = SymbolPairId(sp);
    m_order_books.emplace(spid, OrderBook(NAME, spid, SENT_PRECISIONS.at(spid)));
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
    //std::cout << msg_json << std::endl;
    if (!msg_json.is_array()) {
      // It is most probably a publication or response
      // TODO:
      return;
    }
    auto channel_name = msg_json[msg_json.size() - 2];
    const auto& symbol = msg_json[msg_json.size() - 1].get<std::string>();
    if (channel_name == "ticker") {
      m_exchange_listener->OnTicker(ParseTicker(msg_json[1], symbol));
    } else if (channel_name.get<std::string>().find("book") != std::string::npos) {
      SymbolPair symbol_pair{symbol};
      SymbolPairId pair_id = SymbolPairId(symbol_pair);
      if (m_order_books.count(pair_id) < 1) {
        throw std::runtime_error("Order book update for an unexpected symbol pair");
      }
      OrderBook& ob = m_order_books.at(pair_id);
      bool is_valid = false;
      auto book_obj = msg_json[1];
      if (book_obj.contains("as")) {
        is_valid = true;
        OnOrderBookSnapshot(ob, book_obj);
      } else if (book_obj.contains("a") || book_obj.contains("b")) {
        is_valid = OnOrderBookUpdate(ob, book_obj);
        if (msg_json.size() == 5) {
          is_valid = OnOrderBookUpdate(ob, msg_json[2]);
        }
      } else {
        BOOST_LOG_TRIVIAL(error) << "Unexpected book message!";
      }
      if (is_valid) {
        auto now = system_clock::now();
        system_clock::duration tp = now.time_since_epoch();
        microseconds us = duration_cast<microseconds>(tp);
        m_tickers_watcher.Set(ob.GetSymbolPairId(), us.count());
        m_exchange_listener->OnOrderBookUpdate(ob);
      } else {
        ob.clear();
        // TODO: implement state machine, with subscription class objects in a vector as a state,
        // with which subscriptions can be re-created upon receiving unsubscribed publication
        WebsocketClient::send(OrderBookSubscribeMsg(symbol));
      }
    }
  }

  Ticker ParseTicker(json data, const std::string& symbol) {
      Ticker ticker;
      ticker.m_bid = std::stod(data["b"][0].get<std::string>());
      ticker.m_bid_vol = std::optional<double>(std::stod(data["b"][2].get<std::string>()));
      ticker.m_ask = std::stod(data["a"][0].get<std::string>());
      ticker.m_ask_vol = std::optional<double>(std::stod(data["a"][2].get<std::string>()));
      ticker.m_source_ts = std::nullopt; // not provided
      // TODO: perhaps generate timestamp in base class and pass it to this method
      auto now = system_clock::now();
      system_clock::duration tp = now.time_since_epoch();
      microseconds us = duration_cast<microseconds>(tp);
      ticker.m_arrived_ts = us.count();
      ticker.m_exchange = NAME;
      static uint64_t last_ticker_id = 0;
      ticker.m_id = last_ticker_id++;
      // TODO: use enum
      ticker.m_symbol = symbol;
      m_tickers_watcher.Set(SymbolPair(ticker.m_symbol), ticker.m_arrived_ts);
      return ticker;
  }

  void OnOrderBookSnapshot(OrderBook& ob, const json& snapshot_obj) {
    BOOST_LOG_TRIVIAL(debug) << "Received order book snapshot";
    const auto& precision_settings = SENT_PRECISIONS.at(ob.GetSymbolPairId());
    for (const json& lvl : snapshot_obj["as"]) {
      // Upsert price level
      PriceLevel pl = ParsePriceLevel(lvl, precision_settings);
      ob.UpsertAsk(pl);
    }
    for (const json& lvl : snapshot_obj["bs"]) {
      // Upsert price level
      PriceLevel pl = ParsePriceLevel(lvl, precision_settings);
      ob.UpsertBid(pl);
    }
  }

  bool OnOrderBookUpdate(OrderBook& ob, const json& update_obj) {
    const auto& precision_settings = ob.GetPrecisionSettings();
    if (update_obj.contains("a")) {
      for (const json& lvl : update_obj["a"]) {
        // Upsert price level
        PriceLevel pl = ParsePriceLevel(lvl, precision_settings);
        if (pl.GetVolume() == 0.0) {
          ob.DeleteAsk(pl.GetPrice());
        } else {
          ob.UpsertAsk(pl);
        }
      }
    }
    if (update_obj.contains("b")) {
      for (const json& lvl : update_obj["b"]) {
        // Upsert price level
        PriceLevel pl = ParsePriceLevel(lvl, precision_settings);
        if (pl.GetVolume() == 0.0) {
          ob.DeleteBid(pl.GetPrice());
        } else {
          ob.UpsertBid(pl);
        }
      }
    }
    //std::cout << m_order_book << std::endl;
    if (update_obj.contains("c")) {
      std::string crc32_in = CalculateChecksumInput(ob);
      //std::cout << crc32_in << std::endl;
      boost::crc_32_type crc;
      crc.process_bytes(crc32_in.c_str(), crc32_in.size());
      if (crc.checksum() != std::stoul(update_obj["c"].get<std::string>())) {
        BOOST_LOG_TRIVIAL(debug) << "Wrong checksum!";
        return false;
      }
    }
    //std::cout << m_order_book << std::endl;
    //std::cout << "Best ask: " << m_order_book.GetBestAsk() << ", best bid: " << m_order_book.GetBestBid() << std::endl;
    return true;
  }

  PriceLevel ParsePriceLevel(const json& lvl, const PrecisionSettings& precision_settings) {
    // Parse price
    auto p = lvl[0].get<std::string>();
    size_t dot_pos = p.find('.');
    if (p.size() - 1 - dot_pos != precision_settings.m_price_precision) {
      throw std::runtime_error("Unexpected price precision");
    }
    double price = std::stod(p);
    price_t price_lvl = price_t(uint64_t(price * quick_pow10(precision_settings.m_price_precision)));

    // Parse timestamp
    auto ts = lvl[2].get<std::string>();
    dot_pos = ts.find('.');
    if (ts.size() - 1 - dot_pos != precision_settings.m_timestamp_precision) {
      throw std::runtime_error("Unexpected timestamp precision");
    }
    ts.erase(dot_pos, 1);

    return PriceLevel(price_lvl, std::stod(lvl[1].get<std::string>()), std::stoull(ts));
  }

  std::string CalculateChecksumInput(const OrderBook& ob) {
    const auto& precision_settings = ob.GetPrecisionSettings();
    std::vector<std::string> partial_input;
    const auto& asks = ob.GetAsks(); 
    std::transform(asks.rbegin(), asks.rend(), std::back_inserter(partial_input),
        [&](const PriceLevel& pl) -> std::string {
          return std::to_string(uint64_t(pl.GetPrice())) + std::to_string(uint64_t(pl.GetVolume()*quick_pow10(precision_settings.m_volume_precision)));
        });
    const auto& bids = ob.GetBids(); 
    std::transform(bids.rbegin(), bids.rend(), std::back_inserter(partial_input),
        [&](const PriceLevel& pl) -> std::string {
          return std::to_string(uint64_t(pl.GetPrice())) + std::to_string(uint64_t(pl.GetVolume()*quick_pow10(precision_settings.m_volume_precision)));
        });
    std::string res;
    for (const auto &piece : partial_input) res += piece;
    return res;
  }
private:
  ExchangeListener* m_exchange_listener;
  std::vector<std::string> m_subscription_msg;
  std::unordered_map<SymbolPairId, OrderBook> m_order_books;
  TickersWatcher m_tickers_watcher;
};

const std::unordered_map<SymbolPairId, PrecisionSettings> KrakenWebsocketClient::SENT_PRECISIONS = {
  // price_precision, volume_precision, timestamp_precision
  {SymbolPairId::BTC_USDT, PrecisionSettings(5, 8, 6)},
  {SymbolPairId::EOS_BTC, PrecisionSettings(8, 8, 6)},
  {SymbolPairId::EOS_USDT, PrecisionSettings(6, 8, 6)},
  {SymbolPairId::ADA_BTC, PrecisionSettings(9, 8, 6)},
  {SymbolPairId::ADA_USDT, PrecisionSettings(8, 8, 6)},
  {SymbolPairId::ETH_BTC, PrecisionSettings(6, 8, 6)},
  {SymbolPairId::EOS_ETH, PrecisionSettings(8, 8, 6)},
  {SymbolPairId::ETH_USDT, PrecisionSettings(5, 8, 6)}
};