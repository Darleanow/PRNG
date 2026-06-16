#pragma once

#include <cstdint>

struct LFSR {
  uint32_t val;
};

namespace lfsr {

inline constexpr uint32_t TAPS = (1u << 22) | (1u << 2) | (1u << 1) | (1u << 0);
unsigned step(LFSR &lfsr, uint32_t taps);

}; // namespace lfsr

namespace solver {

uint64_t solve_for_k(std::size_t k, uint8_t *observed);
int run_lfsr_poc();

} // namespace solver
