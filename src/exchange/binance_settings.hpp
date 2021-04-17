#pragma once

#include "json/json.hpp"

#include <boost/log/trivial.hpp>

using json = nlohmann::json;

class BinanceSettings {
public:
  BinanceSettings(const json& payload) {
    ParseExchangeInfo(payload);
  }


private:
  void ParseExchangeInfo(const json& payload) {
    BOOST_LOG_TRIVIAL(debug) << "Got Binance exchange info payload: " << payload;
  }
};