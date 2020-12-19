#pragma once
#include "ticker.h"

class TickerConsumer {
public:
  virtual void Consume(const RawTicker& ticker) = 0;
};