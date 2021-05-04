#pragma once

#include <chrono>
#include <string>
#include <thread>

using namespace std::chrono;

class Watcher {
public:
  Watcher(uint interval_ms) : m_interval_ms(interval_ms) {}
  virtual ~Watcher() {}

  void Start() {
    std::thread th(&Watcher::Watch, this);
    th.detach();
  }

protected:
  virtual void Watch() {
    while(true) {
      WatchImpl();
      std::this_thread::sleep_for(std::chrono::milliseconds(m_interval_ms));
    }
  }

  virtual void WatchImpl() = 0;

protected:
  const uint m_interval_ms;
};