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
          m_tickers_watcher(30000, EXCHANGE_NAME, this),
          m_symbol_pair(SymbolPair::FromBinanceString(symbol_settings.symbol)),
          m_order_book(EXCHANGE_NAME, m_symbol_pair, 1000, PrecisionSettings(symbol_settings.price_precision, symbol_settings.order_precision, 3)) {
        //m_tickers_watcher.Start();
  }

private:
  virtual void OnOpen(websocketpp::connection_hdl conn) override {
    BOOST_LOG_TRIVIAL(trace) << "BinanceOrderBookStream::OnOpen begin";
    m_book_snapshot_future = std::async(std::launch::async, &BinanceOrderBookStream::GetOrderBookSnapshotInitial, this);
    BOOST_LOG_TRIVIAL(trace) << "Getting snapshot";
    m_exchange_listener->OnConnectionOpen(EXCHANGE_NAME);
    BOOST_LOG_TRIVIAL(trace) << "BinanceOrderBookStream::OnOpen end";
  }

  virtual void OnClose(websocketpp::connection_hdl conn) override {
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
          BOOST_LOG_TRIVIAL(info) << "Got order book snapshot: " << snapshot_json;
          m_last_update_id = snapshot_json["lastUpdateId"].get<uint64_t>();
          UpdateOrderBookFromSnapshot(snapshot_json);
          // Replay updates from buffer
          if (!m_depth_updates.empty()) {
            // Validation
            size_t i = 0;
            uint64_t final_update_id = m_depth_updates[i]["u"].get<uint64_t>();
            uint64_t first_update_id = m_depth_updates[i]["U"].get<uint64_t>();
            while (!(first_update_id <= m_last_update_id + 1 && final_update_id >= m_last_update_id + 1) && ++i < m_depth_updates.size()) {
              final_update_id = m_depth_updates[i]["u"].get<uint64_t>();
              first_update_id = m_depth_updates[i]["U"].get<uint64_t>();
            }
            if (i >= m_depth_updates.size()) {
              BOOST_LOG_TRIVIAL(info) << "No buffered events to process";
            }
            for (; i < m_depth_updates.size(); ++i) {
              const json& depth_update = m_depth_updates[i];
              uint64_t final_update_id = depth_update["u"].get<uint64_t>();
              if (final_update_id <= m_last_update_id) {
                continue;
              }
              UpdateOrderBookFromDiff(depth_update);
            }
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
      if (m_last_update_id + 1 != first_update_id) {
        BOOST_LOG_TRIVIAL(error) << "Missing order book update!";
        // TODO: request snapshot again ?
      }
      UpdateOrderBookFromDiff(msg_json);
      m_last_update_id = msg_json["u"].get<uint64_t>();
      //m_tickers_watcher.Set(SymbolPair(ticker.symbol), 0, 0);
      m_exchange_listener->OnOrderBookUpdate(m_order_book);
  }

private:
  void UpdateOrderBookFromSnapshot(const json& order_book_update) {
    for (const json& bid : order_book_update["bids"]) {
      ProcessBid(bid);
    }
    for (const json& ask : order_book_update["asks"]) {
      ProcessAsk(ask);
    }
  }

  void UpdateOrderBookFromDiff(const json& diff_event) {
    for (const json& bid : diff_event["b"]) {
      ProcessBid(bid);
    }
    for (const json& ask : diff_event["a"]) {
      ProcessAsk(ask);
    }
  }

  void ProcessBid(const json& bid) {
    const auto& precision_settings = m_order_book.GetPrecisionSettings();
    const auto& price_str = bid[0].get<std::string>();
    price_t price_lvl = price_t(uint64_t(std::stod(price_str) * cryptobot::quick_pow10(precision_settings.m_price_precision)));
    double qty = std::stod(bid[1].get<std::string>());
    if (qty == 0) {
      m_order_book.DeleteBid(price_lvl);
    }
    PriceLevel level(price_lvl, qty, 0);
    m_order_book.UpsertBid(level);
  }

  void ProcessAsk(const json& ask) {
    const auto& precision_settings = m_order_book.GetPrecisionSettings();
    const auto& price_str = ask[0].get<std::string>();
    price_t price_lvl = price_t(uint64_t(std::stod(price_str) * cryptobot::quick_pow10(precision_settings.m_price_precision)));
    double qty = std::stod(ask[1].get<std::string>());
    if (qty == 0) {
      m_order_book.DeleteAsk(price_lvl);
    }
    PriceLevel level(price_lvl, std::stod(ask[1].get<std::string>()), 0);
    m_order_book.UpsertAsk(level);
  }

  json GetOrderBookSnapshotInitial() {
    // Delay for a second
    std::this_thread::sleep_for(std::chrono::seconds(5));
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
  uint64_t m_last_update_id;
  std::future<json> m_book_snapshot_future;
  std::vector<json> m_depth_updates;
};