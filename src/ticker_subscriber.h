#pragma once

#include "ticker.h"

class TickerSubscriber {
public:
  virtual void OnTicker(const Ticker& ticker) = 0;
  virtual void OnDisconnected(const std::string& exchange_name) = 0;
};