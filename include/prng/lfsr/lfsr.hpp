#pragma once

#include <cstdint>

struct LFSR {
  unsigned int val : 4;
};

constexpr uint32_t TAPS = (1u << 3) | (1u << 2);

unsigned step(LFSR &lfsr);
