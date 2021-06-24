#pragma once

#include <cstddef>

struct PrecisionSettings {
  PrecisionSettings(size_t pp, size_t vp, size_t tp) : m_price_precision(pp), m_volume_precision(vp), m_timestamp_precision(tp) {}

  size_t m_price_precision;
  size_t m_volume_precision;
  size_t m_timestamp_precision;
};