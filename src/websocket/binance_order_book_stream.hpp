#include "tickers_watcher.hpp"

#include "exchange/exchange_listener.h"
#include "http/binance_client.hpp"
#include "model/order_book.h"
#include "serialization_utils.hpp"
#include "websocket/websocket_client.hpp"

#include "json/json.hpp"

#include <chrono>
#include <future>
#include <optional>
#include <string>
#include <thread>

using namespace std::chrono;

using json = nlohmann::json;

class BinanceOrderBookStream : public WebsocketClient {
public:
  inline static const std::string EXCHANGE_NAME = "binance";

  BinanceOrderBookStream(const SymbolPairSettings& symbol_settings, BinanceClient* binance_client, ExchangeListener* exchange_listener)
      : WebsocketClient("wss://stream.binance.com:9443/ws/" + symbol_settings.GetLowerCaseSymbol() + "@depth", EXCHANGE_NAME), m_binance_client(binance_client), m_exchange_listener(exchange_listener),
          m_tickers_watcher(30000, "binance_order_book", this),
          m_symbol_pair(SymbolPair::FromBinanceString(symbol_settings.symbol)),
          m_order_book(EXCHANGE_NAME, m_symbol_pair, 1000, PrecisionSettings(symbol_settings.price_precision, symbol_settings.order_precision, 3)) {
        m_tickers_watcher.Start();
  }

private:
  virtual void OnOpen(websocketpp::connection_hdl) override {
    BOOST_LOG_TRIVIAL(trace) << "BinanceOrderBookStream::OnOpen begin";
    m_book_snapshot_future = std::async(std::launch::async, &BinanceOrderBookStream::GetOrderBookSnapshotInitial, this);
    BOOST_LOG_TRIVIAL(trace) << "Getting snapshot";
    m_exchange_listener->OnConnectionOpen(EXCHANGE_NAME);
    BOOST_LOG_TRIVIAL(trace) << "BinanceOrderBookStream::OnOpen end";
  }

  virtual void OnClose(websocketpp::connection_hdl) override {
    m_exchange_listener->OnConnectionClose(EXCHANGE_NAME);
  }

  virtual void OnMessage(websocketpp::connection_hdl, client::message_ptr msg) override {
      auto msg_json = json::parse(msg->get_payload());
      BOOST_LOG_TRIVIAL(trace) << "Binance order book websocket message: " << msg_json;
      if (!msg_json.is_object()
          || !utils::check_message(msg_json, {"E", "U", "a", "b", "e", "s", "u"})) {
        BOOST_LOG_TRIVIAL(warning) << "Binance: Not an expected ticker object: " << msg_json << std::endl;
        return;
      }
      if (m_book_snapshot_future.valid()) {
        if (m_book_snapshot_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
          json snapshot_json = m_book_snapshot_future.get();
          BOOST_LOG_TRIVIAL(debug) << "Got order book snapshot: " << snapshot_json;
          uint64_t last_update_id = snapshot_json["lastUpdateId"].get<uint64_t>();
          m_previous_update_id = last_update_id;
          m_order_book.Update(DeserializeOrderBookSnapshot(snapshot_json));
          m_exchange_listener->OnOrderBookUpdate(m_order_book);
          // Replay updates from buffer
          if (!m_depth_updates.empty()) {
            // Validation
            size_t i = 0;
            uint64_t final_update_id = m_depth_updates[i]["u"].get<uint64_t>();
            uint64_t first_update_id = m_depth_updates[i]["U"].get<uint64_t>();
            while (!(first_update_id <= last_update_id + 1 && final_update_id >= last_update_id + 1) && ++i < m_depth_updates.size()) {
              final_update_id = m_depth_updates[i]["u"].get<uint64_t>();
              first_update_id = m_depth_updates[i]["U"].get<uint64_t>();
            }
            if (i >= m_depth_updates.size()) {
              BOOST_LOG_TRIVIAL(info) << "No buffered events to process";
            }
            for (; i < m_depth_updates.size(); ++i) {
              const json& depth_update = m_depth_updates[i];
              uint64_t final_update_id = depth_update["u"].get<uint64_t>();
              if (final_update_id <= last_update_id) {
                continue;
              }
              m_order_book.Update(DeserializeOrderBookUpdate(depth_update));
              m_exchange_listener->OnOrderBookUpdate(m_order_book);
              m_previous_update_id = final_update_id;
            }
            m_depth_updates.clear();
          } else {
            BOOST_LOG_TRIVIAL(warning) << "No order book events when requesting snapshot";
          }
        } else {
          BOOST_LOG_TRIVIAL(info) << "Buffering order book update event";
          m_depth_updates.push_back(msg_json);
          return;
        }
      }
      uint64_t first_update_id = msg_json["U"].get<uint64_t>();
      if (m_previous_update_id + 1 < first_update_id) {
        BOOST_LOG_TRIVIAL(warning) << "Missing order book update, requesting snapshot";
        m_order_book.clear();
        m_book_snapshot_future = std::async(std::launch::async, &BinanceOrderBookStream::GetOrderBookSnapshot, this);
        return;
      }
      m_previous_update_id = msg_json["u"].get<uint64_t>();
      OrderBookUpdate update = DeserializeOrderBookUpdate(msg_json);
      m_tickers_watcher.Set(SymbolPair(update.symbol), update.arrived_ts, update.arrived_ts);
      m_order_book.Update(DeserializeOrderBookUpdate(msg_json));
      m_exchange_listener->OnOrderBookUpdate(m_order_book);
  }

private:
  OrderBookUpdate DeserializeOrderBookSnapshot(const json& snapshot_json) { 
    OrderBookUpdate ob_update;
    ob_update.last_update_id = snapshot_json["lastUpdateId"].get<uint64_t>();
    ob_update.is_snapshot = true;
    auto now = system_clock::now();
    system_clock::duration tp = now.time_since_epoch();
    microseconds us = duration_cast<microseconds>(tp);
    ob_update.arrived_ts = us.count();
    for (const json& bid : snapshot_json["bids"]) {
      OrderBookUpdate::Level lvl;
      lvl.price = bid[0].get<std::string>();
      lvl.volume = bid[1].get<std::string>();
      ob_update.bids.push_back(std::move(lvl));
    }
    for (const json& ask : snapshot_json["asks"]) {
      OrderBookUpdate::Level lvl;
      lvl.price = ask[0].get<std::string>();
      lvl.volume = ask[1].get<std::string>();
      ob_update.asks.push_back(std::move(lvl));
    }
    return ob_update;
  }

  OrderBookUpdate DeserializeOrderBookUpdate(const json& update_json) { 
    OrderBookUpdate ob_update;
    ob_update.symbol = SymbolPair::FromBinanceString(update_json["s"].get<std::string>());
    ob_update.exchange = EXCHANGE_NAME;
    ob_update.last_update_id = update_json["u"].get<uint64_t>();
    ob_update.is_snapshot = false;
    auto now = system_clock::now();
    system_clock::duration tp = now.time_since_epoch();
    microseconds us = duration_cast<microseconds>(tp);
    ob_update.arrived_ts = us.count();
    for (const json& bid : update_json["b"]) {
      OrderBookUpdate::Level lvl;
      lvl.price = bid[0].get<std::string>();
      lvl.volume = bid[1].get<std::string>();
      ob_update.bids.push_back(std::move(lvl));
    }
    for (const json& ask : update_json["a"]) {
      OrderBookUpdate::Level lvl;
      lvl.price = ask[0].get<std::string>();
      lvl.volume = ask[1].get<std::string>();
      ob_update.asks.push_back(std::move(lvl));
    }
    return ob_update;
  }

  json GetOrderBookSnapshotInitial() {
    // Delay for a second
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return GetOrderBookSnapshot();
  }

  json GetOrderBookSnapshot() {
    Result<json> res = m_binance_client->GetOrderBookSnapshot(SymbolPairId(m_symbol_pair), BinanceOrderBookDepth::DEPTH_1000);
    if (!res) {
      // TODO: retrials implemented on http client side
      throw std::runtime_error("Getting order book snapshot failed");
    }
    return res.Get();
  }
private:
  BinanceClient* m_binance_client;
  ExchangeListener* m_exchange_listener;
  TickersWatcher m_tickers_watcher;
  SymbolPair m_symbol_pair;
  OrderBook m_order_book;
  uint64_t m_previous_update_id;
  std::future<json> m_book_snapshot_future;
  std::vector<json> m_depth_updates;
};