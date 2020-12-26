#pragma once
#include "ticker.h"

template <typename T>
class Consumer {
public:
  virtual void Consume(const T& ticker) = 0;
};