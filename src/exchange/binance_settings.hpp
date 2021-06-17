#pragma once

#include "symbol_pair_settings.hpp"
#include "utils/string.h"

#include <cassert>

#include "json/json.hpp"

#include <boost/log/trivial.hpp>

using json = nlohmann::json;

class BinanceSettings {
public:
  static BinanceSettings Create(const json& payload) {
    return BinanceSettings(payload);
  }

  const SymbolPairSettings& GetPairSettings(SymbolPairId spid) const {
    return m_pair_settings.at(spid);
  }
private:
  BinanceSettings(const json& payload) {
    ParseExchangeInfo(payload);
  }

  void ParseExchangeInfo(const json& payload) {
    BOOST_LOG_TRIVIAL(trace) << "Got Binance exchange info payload: " << payload;
    const auto& symbols = payload["symbols"];
    for (const auto& symbol_json : symbols) {
      SymbolPairSettings sps;
      sps.symbol = symbol_json["symbol"].get<std::string>();
      sps.base_asset = symbol_json["baseAsset"].get<std::string>();
      sps.quote_asset = symbol_json["quoteAsset"].get<std::string>();
      sps.quote_precision = symbol_json["quotePrecision"].get<size_t>();
      sps.base_asset_precision = symbol_json["baseAssetPrecision"].get<size_t>();
      sps.quote_asset_precision = symbol_json["quoteAssetPrecision"].get<size_t>();

      const auto& filters = symbol_json["filters"];
      for (const auto& filter : filters) {
        const std::string& filter_type = filter["filterType"].get<std::string>();
        if (filter_type == "PRICE_FILTER") {
          sps.min_price = std::stod(filter["minPrice"].get<std::string>());
          sps.max_price = std::stod(filter["maxPrice"].get<std::string>());
          const auto& tick_size_str = filter["tickSize"].get<std::string>();
          sps.tick_size = std::stod(tick_size_str);
          sps.price_precision = cryptobot::precision_from_string(tick_size_str);
        }
        if (filter_type == "LOT_SIZE") {
          sps.min_qty = std::stod(filter["minQty"].get<std::string>());
          sps.max_qty = std::stod(filter["maxQty"].get<std::string>());
          const auto& step_size_str = filter["stepSize"].get<std::string>();
          sps.step_size = std::stod(step_size_str);
          sps.order_precision = cryptobot::precision_from_string(step_size_str);
        }
      }
      SymbolPair sp = SymbolPair::FromBinanceString(sps.symbol);
      SymbolPairId spid = SymbolPairId(sp);
      if (spid == SymbolPairId::UNKNOWN) {
        BOOST_LOG_TRIVIAL(trace) << "BinanceSettings: Symbol pair unknown";
        continue;
      }
      m_pair_settings.emplace(spid, sps);
    }
  }

private:
  // TODO: eventually use string to identify pairs instead of enum;
  // using enum is very small speed gain and a big maintanance effort.
  // On the other hand naming differs from exchange to exchange...
  std::unordered_map<SymbolPairId, SymbolPairSettings> m_pair_settings;
};