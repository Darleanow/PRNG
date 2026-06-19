/**
 * @file mt.hpp
 * @brief MT19937 implementation and full state-cloning attack.
 */

#pragma once

#include <cstdint>

/**
 * @brief MT19937 internal state (624 x 32-bit words + index).
 *
 * Initialise with mt::seed before calling mt::next. Default-constructed state
 * has index set to 624 so the first call to mt::next triggers a twist rather
 * than producing output from an uninitialised state array.
 */
struct MT19937 {
  uint32_t state[624] = {};
  int      index      = 624;
};

/** @brief MT19937 core functions. */
namespace mt {

/**
 * @brief Seeds the generator from a 32-bit value.
 *
 * Uses the Knuth multiplicative initialisation sequence from the reference
 * implementation (Matsumoto & Nishimura, 1998).
 *
 * @param mt    Generator to initialise.
 * @param seed  32-bit seed value.
 */
void seed(MT19937 &mt, uint32_t seed);

/**
 * @brief Generates the next 32-bit output word.
 *
 * Triggers a twist when the state array is exhausted (index >= 624).
 *
 * @param mt  Generator state, updated in-place.
 * @return    Next pseudo-random 32-bit value.
 */
uint32_t next(MT19937 &mt);

} // namespace mt

/** @brief State-cloning attack against MT19937. */
namespace mt_solver {

/**
 * @brief Inverts the MT19937 tempering transform on a single output word.
 *
 * The four mixing operations applied by mt::next are all individually
 * invertible. Undoing them in reverse order recovers the raw state word.
 *
 * @param y  Tempered output word.
 * @return   Untempered state word.
 */
uint32_t untemper(uint32_t y);

/**
 * @brief Runs the full MT19937 cloning PoC.
 *
 * Observes 624 consecutive output words, reconstructs the full internal state
 * via untemper, then predicts the next 100 outputs and verifies them.
 *
 * @return 0 on success, non-zero if any prediction mismatches.
 */
int run_mt_poc();

} // namespace mt_solver
