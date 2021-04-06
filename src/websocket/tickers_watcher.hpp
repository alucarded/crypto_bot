#pragma once

#include "utils/spinlock.hpp"

#include <chrono>
#include <string>
#include <thread>

using namespace std::chrono;

class TickersWatcher {
public:
  TickersWatcher(const std::string& exchange) : m_exchange(exchange) {

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
      for (const auto& p : m_last_arrived) {
        auto ms_since_last = duration_cast<microseconds>(tp).count() - p.second;
        BOOST_LOG_TRIVIAL(info) << m_exchange << ": " << p.first << " arrived " << std::to_string(ms_since_last/1000.0) << " ms ago";
      }
      m_last_arrived_lock.unlock();
      std::this_thread::sleep_for(std::chrono::milliseconds(30000));
    }
  }

protected:
  std::unordered_map<SymbolPairId, uint64_t> m_last_arrived;
  cryptobot::spinlock m_last_arrived_lock;
  const std::string m_exchange;
};