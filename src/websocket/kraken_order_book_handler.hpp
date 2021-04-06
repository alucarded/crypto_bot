#include "model/order_book.h"
#include "model/symbol.h"
#include "utils/math.hpp"
#include "utils/string.hpp"

#include <boost/crc.hpp>
#include <boost/log/trivial.hpp>
#include "json/json.hpp"

#include <algorithm>
#include <string>

class KrakenOrderBookHandler {
public:
  KrakenOrderBookHandler(bool validate_checksum) : m_with_checksum_validation(validate_checksum) {

  }

  bool OnOrderBookMessage(const json& msg_json, OrderBook& ob) {
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
    return is_valid;
  }

private:

  void OnOrderBookSnapshot(OrderBook& ob, const json& snapshot_obj) {
    BOOST_LOG_TRIVIAL(debug) << "Received order book snapshot";
    // Clear order book first
    ob.clear();
    const auto& precision_settings = ob.GetPrecisionSettings();
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
    BOOST_LOG_TRIVIAL(trace) << "Order book: " << ob;
    if (m_with_checksum_validation && update_obj.contains("c")) {
      std::string crc32_in = CalculateChecksumInput(ob);
      BOOST_LOG_TRIVIAL(trace) << "Checksum: " << crc32_in;
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
  bool m_with_checksum_validation;
};