/**
 * @file lfsr.cpp
 * @brief Implementation of the LFSR logic and the algebraic reconstruction solver.
 */

#include "prng/lfsr/lfsr.hpp"
#include <bit>
#include <spdlog/spdlog.h>

namespace lfsr {

unsigned step(LFSR &lfsr, uint32_t taps) {
  unsigned out      = lfsr.val & 1;
  unsigned feedback = std::popcount(lfsr.val & taps) & 1;
  lfsr.val          = (lfsr.val >> 1) | (feedback << 31);
  return out;
}

} // namespace lfsr

namespace solver {

/**
 * @brief In-place Gauss-Jordan elimination over GF(2) on a packed row system.
 *
 * Each row is a 64-bit word where bits 0–(n-1) are coefficients and bit 32
 * is the RHS. Rows are reduced to reduced row echelon form; after the call,
 * row @c col has exactly bit @c col set in the coefficient field.
 *
 * @param rows  Array of @p n packed equation rows, modified in-place.
 * @param n     System dimension (number of unknowns and equations).
 */
static void gauss_jordan(uint64_t *rows, size_t n) {
  for (size_t col = 0; col < n; col++) {
    size_t pivot = n;
    for (size_t i = col; i < n; i++) {
      if ((rows[i] >> col) & 1) { pivot = i; break; }
    }
    if (pivot == n) {
      spdlog::error("Singular system at column {}", col);
      return;
    }
    std::swap(rows[col], rows[pivot]);
    for (size_t i = 0; i < n; i++) {
      if (i != col && ((rows[i] >> col) & 1))
        rows[i] ^= rows[col];
    }
  }
}

/**
 * @brief Extracts the RHS solution vector from a fully reduced row system.
 *
 * Assumes @p rows is already in reduced row echelon form (post gauss_jordan).
 * Reads bit 32 of each row @c col and assembles the result into a uint32_t.
 *
 * @param rows  Reduced row array.
 * @param n     Number of rows/unknowns (must be ≤ 32).
 * @return      Packed solution: bit @c col = value of unknown @c col.
 */
static uint32_t extract_rhs(uint64_t *rows, size_t n) {
  uint32_t result = 0;
  for (size_t col = 0; col < n; col++)
    result |= static_cast<uint32_t>((rows[col] >> 32) & 1) << col;
  return result;
}

uint64_t solve_for_k(std::size_t k, uint8_t *observed) {
  uint64_t line = 0;
  for (std::size_t i = 0; i < 32; i++)
    line |= static_cast<uint64_t>(observed[k + i]) << i;
  line |= static_cast<uint64_t>(observed[k + 32]) << 32;
  spdlog::trace("line {:2}: {:#018x}", k, line);
  return line;
}

/**
 * @brief Builds and solves the tap-recovery linear system from observed bits.
 *
 * Constructs N equations of the form s[k+32] = c₀·s[k] ⊕ … ⊕ c₃₁·s[k+31]
 * using solve_for_k, then recovers the unknown tap mask via Gauss-Jordan.
 *
 * @param observed  Output bit array; must have at least 2·N entries.
 * @return          Recovered tap mask.
 */
static uint32_t recover_taps(uint8_t *observed) {
  constexpr size_t N = 32;
  uint64_t lines[N];
  for (size_t k = 0; k < N; k++)
    lines[k] = solve_for_k(k, observed);
  spdlog::info("Built {0}x{0} linear system", N);

  gauss_jordan(lines, N);
  spdlog::info("Gauss-Jordan elimination done");

  return extract_rhs(lines, N);
}

/**
 * @brief Computes the symbolic dependency of each output bit on the initial state.
 *
 * Each sym[i] is a bitmask over the 32 initial-state bits: bit j set means
 * output bit i depends on initial state bit j. The first N entries are trivial
 * (sym[i] = 1<<i); subsequent entries follow the recurrence defined by @p taps.
 *
 * @param sym   Output array of size 2·N, filled with dependency masks.
 * @param taps  Feedback tap mask defining the recurrence relation.
 */
static void compute_symbolic(uint32_t *sym, uint32_t taps) {
  constexpr size_t N = 32;
  for (size_t i = 0; i < N; i++)
    sym[i] = 1u << i;
  for (size_t i = N; i < 2 * N; i++) {
    uint32_t fb = 0;
    for (size_t b = 0; b < N; b++)
      if ((taps >> b) & 1) fb ^= sym[i - N + b];
    sym[i] = fb;
  }
}

uint32_t recover_initial_state(uint8_t *observed, uint32_t taps) {
  constexpr size_t N = 32;
  uint32_t sym[2 * N];
  compute_symbolic(sym, taps);

  uint64_t eqs[N];
  for (size_t i = 0; i < N; i++)
    eqs[i] = static_cast<uint64_t>(sym[i]) | (static_cast<uint64_t>(observed[i]) << 32);

  gauss_jordan(eqs, N);
  return extract_rhs(eqs, N);
}

/**
 * @brief Logs and validates the recovered tap mask against the known original.
 *
 * @param recovered  Tap mask returned by recover_taps.
 * @return           0 if recovered matches lfsr::TAPS, 1 otherwise.
 */
static int verify_taps(uint32_t recovered) {
  spdlog::info("Recovered taps: {:#010x}", recovered);
  spdlog::info("Original  taps: {:#010x}", lfsr::TAPS);
  if (recovered != lfsr::TAPS) {
    spdlog::error("Tap recovery failed");
    return 1;
  }
  spdlog::info("Step 1 passed: feedback polynomial fully recovered");
  return 0;
}

/**
 * @brief Logs and validates the recovered initial state against the known seed.
 *
 * Also rejects the 0xFFFFFFFF sentinel returned by recover_initial_state on
 * singular systems.
 *
 * @param recovered  State returned by recover_initial_state.
 * @return           0 if recovered matches 0xDEADBEEF, 1 otherwise.
 */
static int verify_state(uint32_t recovered) {
  spdlog::info("Recovered state: {:#010x}", recovered);
  spdlog::info("Original  state: {:#010x}", 0xDEADBEEFu);
  if (recovered == 0xFFFFFFFFu || recovered != 0xDEADBEEFu) {
    spdlog::error("Initial state recovery failed");
    return 1;
  }
  spdlog::info("Step 2 passed: initial state fully recovered");
  return 0;
}

/**
 * @brief Clones the victim LFSR from recovered state and verifies future output.
 *
 * Advances the clone past the 2·N already-observed bits to synchronise it with
 * the victim, then compares the next PREDICT output bits one by one.
 *
 * @param victim  The original LFSR, positioned right after the observed window.
 * @param state   Recovered initial state to seed the clone from.
 * @param taps    Recovered (or known) feedback tap mask.
 * @return        0 if all predicted bits match, 1 on first mismatch.
 */
static int verify_prediction(LFSR &victim, uint32_t state, uint32_t taps) {
  constexpr size_t N      = 32;
  constexpr size_t PREDICT = 32;

  LFSR clone{state};
  for (size_t i = 0; i < 2 * N; i++)
    lfsr::step(clone, taps);

  for (size_t i = 0; i < PREDICT; i++) {
    uint8_t got      = lfsr::step(clone, taps);
    uint8_t expected = lfsr::step(victim, taps);
    spdlog::debug("predict {:2}: got={} expected={}", i, got, expected);
    if (got != expected) {
      spdlog::error("Prediction mismatch at bit {}", i);
      return 1;
    }
  }
  spdlog::info("Step 3 passed: {} future bits correctly predicted", PREDICT);
  return 0;
}

int run_lfsr_poc() {
  constexpr size_t N = 32;
  spdlog::info("LFSR break over GF(2)");

  LFSR victim{0xDEADBEEF};
  spdlog::debug("seed = {:#010x}", victim.val);

  uint8_t observed[2 * N];
  for (size_t n = 0; n < 2 * N; n++) {
    observed[n] = lfsr::step(victim, lfsr::TAPS);
    spdlog::debug("step {:2}: state={:#010x} out={}", n, victim.val, observed[n]);
  }
  spdlog::info("Observed {} output bits", 2 * N);

  uint32_t taps = recover_taps(observed);
  if (verify_taps(taps)) return 1;

  uint32_t state = recover_initial_state(observed, taps);
  if (verify_state(state)) return 1;

  return verify_prediction(victim, state, taps);
}

} // namespace solver
