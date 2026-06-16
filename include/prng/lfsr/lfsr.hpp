#pragma once

#include <cstdint>

struct LFSR {
  uint32_t val;
};

inline constexpr uint32_t TAPS = (1u << 22) | (1u << 2) | (1u << 1) | (1u << 0);

unsigned step(LFSR &lfsr);
