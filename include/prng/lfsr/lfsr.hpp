/**
 * @file lfsr.hpp
 * @brief Fibonacci LFSR and algebraic reconstruction attack over GF(2).
 */

#pragma once

#include <cstdint>

/**
 * @brief 32-bit Fibonacci LFSR state (shift-right, feedback injected at MSB).
 */
struct LFSR {
  uint32_t val; /**< The 32-bit register value. */
};

/** @brief LFSR core functions and constants. */
namespace lfsr {

/**
 * @brief Primitive polynomial x^32+x^23+x^3+x^2+x+1 tap mask.
 *
 * Bits 0, 1, 2, and 22 are set, corresponding to taps at those positions.
 */
inline constexpr uint32_t TAPS = (1u << 22) | (1u << 2) | (1u << 1) | (1u << 0);

/**
 * @brief Clocks the LFSR one step.
 *
 * Fibonacci topology: shift-right, feedback XORed from tap positions and
 * injected at bit 31. Galois would be the left-shift equivalent.
 *
 * @param lfsr  LFSR state to update in-place.
 * @param taps  Feedback tap mask.
 * @return      Output bit (0 or 1).
 */
unsigned step(LFSR &lfsr, uint32_t taps);

} // namespace lfsr

/** @brief Algebraic reconstruction attack against an unmasked LFSR. */
namespace solver {

/**
 * @brief Encodes one recurrence equation as a packed 64-bit row.
 *
 * Builds: s[k+32] = c₀·s[k] ⊕ … ⊕ c₃₁·s[k+31]
 * Bits 0–31 hold the unknown tap coefficients; bit 32 holds the observed RHS.
 *
 * @param k        Starting index in the observed sequence.
 * @param observed Output bit array; must have at least k+33 entries.
 * @return         Packed 64-bit equation row.
 */
uint64_t solve_for_k(std::size_t k, uint8_t *observed);

/**
 * @brief Recovers the 32-bit initial register state from observed output.
 *
 * Unrolls the recurrence symbolically to express each output bit as a linear
 * combination of the 32 unknown initial-state bits, then solves the resulting
 * 32×32 system via Gauss-Jordan elimination over GF(2).
 *
 * @param observed  Output bit array; must have at least 2·N entries.
 * @param taps      Feedback tap mask (already recovered or known).
 * @return          Recovered initial state, or 0xFFFFFFFF if system is singular.
 */
uint32_t recover_initial_state(uint8_t *observed, uint32_t taps);

/**
 * @brief Runs the full LFSR break PoC.
 * @return 0 on success, non-zero if any step fails.
 */
int run_lfsr_poc();

} // namespace solver
