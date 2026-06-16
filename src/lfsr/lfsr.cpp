/**
 * @file lfsr.cpp
 * @brief Implementation of the LFSR logic and the algebraic reconstruction solver.
 */

#include "prng/lfsr/lfsr.hpp"
#include <bit>
#include <spdlog/spdlog.h>

namespace lfsr {

unsigned step(LFSR &lfsr, uint32_t taps) {
  unsigned out = lfsr.val & 1;
  unsigned feedback = std::popcount(lfsr.val & taps) & 1;
  lfsr.val = (lfsr.val >> 1) | (feedback << 31);
  return out;
}

}; // namespace lfsr

namespace solver {

uint64_t solve_for_k(std::size_t k, uint8_t *observed) {
  uint64_t line = 0;
  for (std::size_t i = 0; i < 32; i++) {
    line |= static_cast<uint64_t>(observed[k + i]) << i;
  }

  line |= static_cast<uint64_t>(observed[k + 32]) << 32;
  spdlog::trace("line {:2}: {:#018x}", k, line);
  return line;
}

int run_lfsr_poc() {
  constexpr size_t N = 32;

  spdlog::info("LFSR break over GF(2)");

  LFSR victim{0xDEADBEEF};
  spdlog::debug("seed = {:#010x}", victim.val);

  uint8_t observed[2 * N];
  for (size_t n = 0; n < 2 * N; n++) {
    observed[n] = lfsr::step(victim, lfsr::TAPS);
    spdlog::debug("step {:2}: state={:#010x} out={}", n, victim.val,
                  observed[n]);
  }
  spdlog::info("Observed {} output bits", 2 * N);

  uint64_t lines[N];
  for (size_t k = 0; k < N; k++) {
    lines[k] = solver::solve_for_k(k, observed);
  }
  spdlog::info("Built {0}x{0} linear system", N);

  for (size_t col = 0; col < N; col++) {
    size_t pivot = N;
    for (size_t i = col; i < N; i++) {
      if ((lines[i] >> col) & 1) {
        pivot = i;
        break;
      }
    }
    if (pivot == N) {
      spdlog::error("Singular system: no pivot in column {}", col);
      return 1;
    }

    std::swap(lines[col], lines[pivot]);
    for (size_t i = 0; i < N; i++) {
      if (i != col && ((lines[i] >> col) & 1)) {
        lines[i] ^= lines[col];
      }
    }
  }
  spdlog::info("Gauss-Jordan elimination done");

  uint32_t recovered = 0;
  for (size_t col = 0; col < N; col++) {
    recovered |= ((lines[col] >> 32) & 1) << col;
  }

  spdlog::info("Recovered taps: {:#010x}", recovered);
  spdlog::info("Original  taps: {:#010x}", lfsr::TAPS);

  if (recovered != lfsr::TAPS) {
    spdlog::error("Recovery failed");
    return 1;
  }
  spdlog::info("Success: taps fully recovered");
  return 0;
}

}; // namespace solver
