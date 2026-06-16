#pragma once

#include <cstdint>

struct LFSR {
  uint32_t val;
};

inline constexpr uint32_t TAPS =
    (1u << 31) | (1u << 30) | (1u << 29) | (1u << 9);
unsigned step(LFSR &lfsr);
