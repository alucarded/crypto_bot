#pragma once

#include "settings.h"
#include "symbol.h"

// TODO: Create ExchangeSettings class, which pulls these from https://api.kraken.com/0/public/AssetPairs
extern const std::unordered_map<SymbolPairId, PrecisionSettings> KRAKEN_ORDER_PRECISIONS_MAP = {
  // price_precision, volume_precision, timestamp_precision
  {SymbolPairId::BTC_USDT, PrecisionSettings(1, 8, 6)},
  {SymbolPairId::DOT_BTC, PrecisionSettings(4, 8, 6)},
  {SymbolPairId::DOT_USDT, PrecisionSettings(4, 8, 6)},
  {SymbolPairId::EOS_BTC, PrecisionSettings(7, 8, 6)},
  {SymbolPairId::EOS_USDT, PrecisionSettings(4, 8, 6)},
  {SymbolPairId::ADA_BTC, PrecisionSettings(8, 8, 6)},
  {SymbolPairId::ADA_USDT, PrecisionSettings(6, 8, 6)},
  {SymbolPairId::ETH_BTC, PrecisionSettings(5, 8, 6)},
  {SymbolPairId::EOS_ETH, PrecisionSettings(6, 8, 6)},
  {SymbolPairId::ETH_USDT, PrecisionSettings(2, 8, 6)},
  {SymbolPairId::XLM_BTC, PrecisionSettings(8, 8, 6)},
  {SymbolPairId::DOGE_USDT, PrecisionSettings(5, 8, 6)},
};