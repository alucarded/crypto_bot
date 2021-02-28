#pragma once
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <utility>
#include <list>

#include "json/json.hpp"

using json = nlohmann::json;

using quantity_t = double;
enum class price_t : uint64_t {};

class PriceLevel {
public:
  PriceLevel() {}
  PriceLevel(price_t p, quantity_t vol, uint64_t ts) : m_price(p),
      m_volume(vol),
      m_timestamp(ts) { }
  PriceLevel(const PriceLevel& o) noexcept : m_volume(o.m_volume),
      m_price(o.m_price),
      m_timestamp(o.m_timestamp) { }
  PriceLevel(PriceLevel&& o) noexcept : m_volume(std::exchange(o.m_volume, 0)),
      m_price(std::move(o.m_price)),
      m_timestamp(std::exchange(o.m_timestamp, 0)) { }
  PriceLevel&& operator=(PriceLevel&& o) {
    m_volume = std::exchange(o.m_volume, 0);
    m_price = std::move(o.m_price);
    m_timestamp = std::exchange(o.m_timestamp, 0);
  }

  static PriceLevel Null() {
    return PriceLevel(price_t(0), 0, 0);
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
	OrderBook() : OrderBook(10) {
  }

	OrderBook(size_t depth) : m_depth(depth) {
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

	const std::list<PriceLevel>& GetBids() const { return m_bids; }
	const std::list<PriceLevel>& GetAsks() const { return m_asks; }

  const PriceLevel& GetBestBid() const {
    if (m_bids.empty()) {
      return PriceLevel::Null();
    }
    return *m_bids.end();
  }

  const PriceLevel& GetBestAsk() const {
    if (m_asks.empty()) {
      return PriceLevel::Null();
    }
    return *m_asks.end();
  }

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
  size_t m_depth;
};

std::ostream& operator<<(std::ostream& os, const PriceLevel& pl) {
  os << "{ \"price\": " << std::to_string(uint64_t(pl.m_price)) << ", \"volume\": " << std::fixed << std::setprecision(8) << pl.m_volume << ", \"timestamp\": " << std::to_string(pl.m_timestamp) << " }";
  return os;
}

std::ostream& operator<<(std::ostream& os, const OrderBook& ob) {
  os << "ASKS:" << std::endl;
  for (const auto& lvl : ob.m_asks) {
    os << lvl << std::endl;
  }
  os << "BIDS:" << std::endl;
  for (const auto& lvl : ob.m_bids) {
    os << lvl << std::endl;
  }
  return os;
}