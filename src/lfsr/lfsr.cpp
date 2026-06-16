#include "prng/lfsr/lfsr.hpp"
#include <bit>

unsigned step(LFSR &lfsr) {
  unsigned out = lfsr.val & 1;
  unsigned feedback = std::popcount(lfsr.val & TAPS) & 1;
  lfsr.val = (lfsr.val >> 1) | (feedback << 31);
  return out;
}
