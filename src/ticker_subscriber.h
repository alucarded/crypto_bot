#pragma once

#include "model/ticker.h"

// TODO: Remove
class TickerSubscriber {
public:
  virtual void OnTicker(const Ticker& ticker) = 0;
  virtual void OnDisconnected(const std::string& exchange_name) = 0;
};