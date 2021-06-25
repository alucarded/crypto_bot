#pragma once

#include "order_book_update.h"
#include "settings.h"
#include "symbol.h"
#include "utils/math.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <utility>
#include <list>

#include "json/json.hpp"

#include <boost/log/trivial.hpp>

using json = nlohmann::json;

using quantity_t = double;
enum class price_t : uint64_t {};

class PriceLevel {
public:
  PriceLevel() {}
  PriceLevel(price_t p, quantity_t vol, uint64_t ts) : m_price(p),
      m_volume(vol),
      m_timestamp(ts) { }
  PriceLevel(const PriceLevel& o) noexcept : m_price(o.m_price),
      m_volume(o.m_volume),
      m_timestamp(o.m_timestamp) { }
  PriceLevel(PriceLevel&& o) noexcept : m_price(std::move(o.m_price)),
      m_volume(std::exchange(o.m_volume, 0)),
      m_timestamp(std::exchange(o.m_timestamp, 0)) { }
  PriceLevel& operator=(PriceLevel&& o) {
    m_price = std::move(o.m_price);
    m_volume = std::exchange(o.m_volume, 0);
    m_timestamp = std::exchange(o.m_timestamp, 0);
    return *this;
  }

  static const PriceLevel& Null() {
    static PriceLevel s_null_level(price_t(0), 0, 0);
    return s_null_level;
  }

  bool operator>(const PriceLevel& rhs) const noexcept {
    return m_price > rhs.m_price;
  }

  bool operator<(const PriceLevel& rhs) const noexcept {
    return m_price < rhs.m_price;
  }

  bool operator==(const PriceLevel& rhs) const noexcept {
    return m_price == rhs.m_price;
  }

  bool operator>(const price_t& rhs) const noexcept {
    return m_price > rhs;
  }

  bool operator<(const price_t& rhs) const noexcept {
    return m_price < rhs;
  }

  bool operator==( const price_t& rhs ) const noexcept {
    return m_price == rhs;
  }

  inline void SetVolume(quantity_t volume) { this->m_volume = volume; }
  inline void SetPrice(price_t price) { this->m_price = price; }
  inline void SetTimestamp(uint64_t timestamp) { this->m_timestamp = timestamp; }

  inline quantity_t GetVolume() const { return m_volume; }
  inline price_t GetPrice() const { return m_price; }
  inline uint64_t GetTimestamp() const { return m_timestamp; }

  friend std::ostream& operator<<(std::ostream& os, const PriceLevel& ob);

private:
  price_t m_price;
  quantity_t m_volume;
  uint64_t m_timestamp;
};

class OrderBook {
public:

	OrderBook(const std::string& name, SymbolPairId symbol, PrecisionSettings precision_settings)
      : OrderBook(name, symbol, 10, precision_settings) {
  }

	OrderBook(const std::string& name, SymbolPairId symbol, size_t depth, PrecisionSettings precision_settings)
      : m_exchange_name(name), m_symbol(symbol), m_depth(depth), m_precision_settings(precision_settings) {
  }

	/**
	 * Upsert bid price level with quantity.
	 *
	 * @param price_level price level
	 */
	void UpsertBid(PriceLevel& price_level) {
    auto it = m_bids.begin();
		bool found = false;
		for (; it != m_bids.end(); ++it) {
			PriceLevel& cprice = *it;
			if (cprice == price_level) {
        cprice = std::move(price_level);
				found = true;
				break;
			} else if (price_level < cprice) {
				break;
			}
		}
		if (!found) {
			m_bids.insert(it, price_level);
		}
    if (m_bids.size() > m_depth) {
      m_bids.pop_front();
    }
  }

  /**
	 * Upsert ask price level with quantity.
	 *
	 * @param price_level price level
	 */
	void UpsertAsk(PriceLevel& price_level) {
		auto it = m_asks.begin();
		bool found = false;
		for (; it != m_asks.end(); ++it) {
			PriceLevel& cprice = *it;
			if (cprice == price_level) {
        cprice = std::move(price_level);
				found = true;
				break;
			} else if (price_level > cprice) {
				break;
			}
		}
		if (!found) {
			m_asks.insert(it, price_level);
		}
    if (m_asks.size() > m_depth) {
      m_asks.pop_front();
    }
  }

  void DeleteBid(price_t price) {
		Delete(m_bids, price);
  }

  void DeleteAsk(price_t price) {
    Delete(m_asks, price);
  }

  void clear() {
    m_asks.clear();
    m_bids.clear();
  }

	const std::list<PriceLevel>& GetBids() const { return m_bids; }
	const std::list<PriceLevel>& GetAsks() const { return m_asks; }

  const PriceLevel& GetBestBid() const {
    if (m_bids.empty()) {
      return PriceLevel::Null();
    }
    return m_bids.back();
  }

  const PriceLevel& GetBestAsk() const {
    if (m_asks.empty()) {
      return PriceLevel::Null();
    }
    return m_asks.back();
  }

  uint64_t GetLatestUpdateTimestamp() const {
    // TODO: use m_last_update ?
    uint64_t ret = 0;
    std::for_each(m_bids.cbegin(), m_bids.cend(), [&](const PriceLevel& pl) {
      ret = std::max<uint64_t>(ret, pl.GetTimestamp());
    });
    std::for_each(m_asks.cbegin(), m_asks.cend(), [&](const PriceLevel& pl) {
      ret = std::max<uint64_t>(ret, pl.GetTimestamp());
    });
    return ret;
  }

  void Update(const OrderBookUpdate& ob_update) {
    m_last_update = ob_update;
    for (const auto& raw_lvl : m_last_update.bids) {
      price_t price_lvl = price_t(uint64_t(std::stod(raw_lvl.price) * cryptobot::quick_pow10(m_precision_settings.m_price_precision)));
      double qty = std::stod(raw_lvl.volume);
      if (qty == 0) {
        DeleteBid(price_lvl);
        continue;
      }
      PriceLevel level(price_lvl, qty, raw_lvl.timestamp.has_value() ? raw_lvl.timestamp.value() : 0);
      UpsertBid(level);
    }
    for (const auto& raw_lvl : m_last_update.asks) {
      price_t price_lvl = price_t(uint64_t(std::stod(raw_lvl.price) * cryptobot::quick_pow10(m_precision_settings.m_price_precision)));
      double qty = std::stod(raw_lvl.volume);
      if (qty == 0) {
        DeleteAsk(price_lvl);
        continue;
      }
      PriceLevel level(price_lvl, qty, raw_lvl.timestamp.has_value() ? raw_lvl.timestamp.value() : 0);
      UpsertAsk(level);
    }
  }

  inline const std::string& GetExchangeName() const { return m_exchange_name; }
  inline SymbolPairId GetSymbolPairId() const { return m_symbol; }
  inline size_t GetDepth() const { return m_depth; }
  inline const PrecisionSettings& GetPrecisionSettings() const { return m_precision_settings; }
  inline const OrderBookUpdate& GetLastUpdate() const { return m_last_update; }

  friend std::ostream& operator<<(std::ostream& os, const OrderBook& ob);

private:
  inline static void Delete(std::list<PriceLevel>& vec, price_t price) {
		auto it = vec.end();
		while (it-- != vec.begin()) {
			PriceLevel& cprice = *it;
			if (cprice == price) {
        vec.erase(it);
				break;
			}
		}
  }

private:
  // Highest bid last
	std::list<PriceLevel> m_bids;
  // Lowest ask last
	std::list<PriceLevel> m_asks;
  const std::string m_exchange_name;
  const SymbolPairId m_symbol;
  const size_t m_depth;
  const PrecisionSettings m_precision_settings;
  OrderBookUpdate m_last_update;
};

std::ostream& operator<<(std::ostream& os, const PriceLevel& pl);
std::ostream& operator<<(std::ostream& os, const OrderBook& ob);