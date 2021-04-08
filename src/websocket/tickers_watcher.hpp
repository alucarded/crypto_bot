#pragma once

#include "utils/spinlock.hpp"
#include "websocket_client.hpp"

#include <algorithm>
#include <chrono>
#include <string>
#include <thread>

using namespace std::chrono;

class TickersWatcher {
public:
  TickersWatcher(const std::string& exchange, uint interval_ms, WebsocketClient* client)
      : m_exchange(exchange), m_interval_ms(interval_ms), m_client(client) {

  }

  void Start() {
    std::thread th(&TickersWatcher::Watch, this);
    th.detach();
  }

  void Set(SymbolPairId spid, uint64_t ts) {
    m_last_arrived_lock.lock();
    m_last_arrived[spid] = ts;
    m_last_arrived_lock.unlock();
  }

protected:
  virtual void Watch() {
    while(true) {
      auto now = system_clock::now();
      system_clock::duration tp = now.time_since_epoch();
      m_last_arrived_lock.lock();
      SymbolPairId max_age_pair;
      double max_age = 0;
      for (const auto& p : m_last_arrived) {
        auto us_since_last = duration_cast<microseconds>(tp).count() - p.second;
        auto ms_since_last = us_since_last/1000.0;
        max_age_pair = p.first;
        max_age = std::max<double>(max_age, ms_since_last);
        BOOST_LOG_TRIVIAL(info) << m_exchange << ": " << p.first << " arrived " << std::to_string(ms_since_last) << " ms ago";
      }
      if (max_age > m_interval_ms) {
        // Close connection if there were no messages for any pairs for too long
        // Websocket client should automatically reconnect
        // TODO: is it thread-safe?
        BOOST_LOG_TRIVIAL(warning) << m_exchange << ": " << "No " << max_age_pair << " data for over " << m_interval_ms << " ms. Closing connection.";
        m_client->close();
      }
      m_last_arrived_lock.unlock();
      std::this_thread::sleep_for(std::chrono::milliseconds(m_interval_ms));
    }
  }

protected:
  std::unordered_map<SymbolPairId, uint64_t> m_last_arrived;
  cryptobot::spinlock m_last_arrived_lock;
  const std::string m_exchange;
  const uint m_interval_ms;
  WebsocketClient* m_client;
};