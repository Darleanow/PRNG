/**
 * @file lfsr.hpp
 * @brief Fibonacci LFSR implementation and algebraic reconstruction attack over GF(2).
 * 
 * Provides a simple 32-bit Linear Feedback Shift Register (LFSR) and an attack
 * to recover its feedback taps given an observed output sequence.
 */

#pragma once

#include <cstdint>

/** 
 * @brief 32-bit Fibonacci LFSR state.
 * 
 * Represents the internal state of the generator. 
 * The state shifts right and injects feedback at bit 31.
 */
struct LFSR {
  uint32_t val; /**< The 32-bit register value */
};

/** @brief LFSR core functions and constants. */
namespace lfsr {

/** 
 * @brief Default feedback tap mask.
 * 
 * Defines the recurrence relation used by the generator. 
 * Bits 0, 1, 2, and 22 are set.
 */
inline constexpr uint32_t TAPS = (1u << 22) | (1u << 2) | (1u << 1) | (1u << 0);

/** 
 * @brief Advances the LFSR by one step.
 * 
 * Computes the new feedback bit, shifts the state, and returns the output bit.
 * 
 * @param lfsr The LFSR state to update.
 * @param taps The feedback tap mask to use.
 * @return The generated output bit (0 or 1).
 */
unsigned step(LFSR &lfsr, uint32_t taps);

} // namespace lfsr

/** @brief Tools for recovering LFSR taps from output. */
namespace solver {

/** 
 * @brief Builds a single linear equation from observed output.
 * 
 * @param k The row index representing the starting point in the sequence.
 * @param observed Array containing the generated bits.
 * @return A 64-bit integer encoding the equation's coefficients and RHS.
 */
uint64_t solve_for_k(std::size_t k, uint8_t *observed);

/** 
 * @brief Runs the complete Proof-of-Concept LFSR break.
 * 
 * Simulates an LFSR, collects its output, constructs a system of linear 
 * equations, and solves it via Gauss-Jordan elimination to recover the taps.
 * 
 * @return 0 on success, non-zero if the attack fails.
 */
int run_lfsr_poc();

} // namespace solver
