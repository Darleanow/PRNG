/**
 * @file mt.cpp
 * @brief MT19937 implementation and state-cloning attack.
 */

#include "prng/mt/mt.hpp"
#include <spdlog/spdlog.h>

namespace mt {

// MT19937 constants (Matsumoto & Nishimura, 1998)
static constexpr int      N          = 624;
static constexpr int      M          = 397;
static constexpr uint32_t MATRIX_A   = 0x9908b0dfu;
static constexpr uint32_t UPPER_MASK = 0x80000000u;
static constexpr uint32_t LOWER_MASK = 0x7fffffffu;

void seed(MT19937 &mt, uint32_t s) {
  mt.state[0] = s;
  for (int i = 1; i < N; i++)
    mt.state[i] = 1812433253u * (mt.state[i - 1] ^ (mt.state[i - 1] >> 30)) + i;
  mt.index = N;
}

/**
 * @brief Regenerates the full 624-word state array.
 *
 * Processes the array in three segments to avoid modulo arithmetic in the
 * hot loop: [0, N-M), [N-M, N-1), and the single wrap-around element at N-1.
 */
static void twist(MT19937 &mt) {
  auto mix = [](uint32_t a, uint32_t b) -> uint32_t {
    uint32_t x = (a & UPPER_MASK) | (b & LOWER_MASK);
    return (x >> 1) ^ ((x & 1) ? MATRIX_A : 0u);
  };

  for (int i = 0; i < N - M; i++)
    mt.state[i] = mt.state[i + M] ^ mix(mt.state[i], mt.state[i + 1]);
  for (int i = N - M; i < N - 1; i++)
    mt.state[i] = mt.state[i + M - N] ^ mix(mt.state[i], mt.state[i + 1]);
  mt.state[N - 1] = mt.state[M - 1] ^ mix(mt.state[N - 1], mt.state[0]);

  mt.index = 0;
}

uint32_t next(MT19937 &mt) {
  if (mt.index >= N)
    twist(mt);

  uint32_t y = mt.state[mt.index++];
  y ^= y >> 11;
  y ^= (y << 7)  & 0x9d2c5680u;
  y ^= (y << 15) & 0xefc60000u;
  y ^= y >> 18;
  return y;
}

} // namespace mt

namespace mt_solver {

/**
 * @brief Inverts a right-shift XOR step: y ^= y >> shift.
 *
 * The top @p shift bits are unchanged; each lower chunk is recovered by
 * XORing with the already-recovered bits above it.
 *
 * @param y      Mixed value.
 * @param shift  Shift amount from the original operation.
 * @return       Value before the mixing step.
 */
static uint32_t invert_right_xor(uint32_t y, int shift) {
  uint32_t result = y;
  for (int i = shift; i < 32; i += shift)
    result = y ^ (result >> shift);
  return result;
}

/**
 * @brief Inverts a left-shift XOR-AND step: y ^= (y << shift) & mask.
 *
 * The bottom @p shift bits are unchanged; each higher chunk is recovered by
 * XORing with the masked, already-recovered bits below it.
 *
 * @param y      Mixed value.
 * @param shift  Shift amount from the original operation.
 * @param mask   AND mask from the original operation.
 * @return       Value before the mixing step.
 */
static uint32_t invert_left_xor(uint32_t y, int shift, uint32_t mask) {
  uint32_t result = y;
  for (int i = shift; i < 32; i += shift)
    result = y ^ ((result << shift) & mask);
  return result;
}

uint32_t untemper(uint32_t y) {
  y = invert_right_xor(y, 18);
  y = invert_left_xor(y,  15, 0xefc60000u);
  y = invert_left_xor(y,   7, 0x9d2c5680u);
  y = invert_right_xor(y, 11);
  return y;
}

/**
 * @brief Reconstructs the full MT19937 state from 624 observed output words.
 *
 * Each output word is the tempered form of one state word. Untemping all 624
 * recovers the state array exactly as it was after the last twist, so the
 * clone will produce identical output from that point on.
 *
 * @param observed  Array of exactly 624 consecutive output words.
 * @param clone     Generator whose state is overwritten with the result.
 */
static void reconstruct_state(uint32_t *observed, MT19937 &clone) {
  for (int i = 0; i < mt::N; i++)
    clone.state[i] = untemper(observed[i]);
  clone.index = mt::N;
}

/**
 * @brief Verifies that @p clone produces the same next 100 outputs as @p victim.
 *
 * @param victim  Original generator, positioned just after the observed window.
 * @param clone   Cloned generator with reconstructed state.
 * @return        0 if all 100 outputs match, 1 on first mismatch.
 */
static int verify_prediction(MT19937 &victim, MT19937 &clone) {
  constexpr int PREDICT = 100;
  for (int i = 0; i < PREDICT; i++) {
    uint32_t got      = mt::next(clone);
    uint32_t expected = mt::next(victim);
    spdlog::debug("predict {:3}: got={:#010x} expected={:#010x}", i, got, expected);
    if (got != expected) {
      spdlog::error("Prediction mismatch at output {}", i);
      return 1;
    }
  }
  spdlog::info("Step 2 passed: {} future outputs correctly predicted", PREDICT);
  return 0;
}

int run_mt_poc() {
  spdlog::info("MT19937 clone attack");

  MT19937 victim;
  mt::seed(victim, 0xDEADBEEF);
  spdlog::debug("seed = {:#010x}", 0xDEADBEEFu);

  uint32_t observed[mt::N];
  for (int i = 0; i < mt::N; i++) {
    observed[i] = mt::next(victim);
    spdlog::debug("observe {:3}: {:#010x}", i, observed[i]);
  }
  spdlog::info("Observed {} output words", mt::N);

  MT19937 clone;
  reconstruct_state(observed, clone);
  spdlog::info("Step 1 passed: internal state fully reconstructed");

  return verify_prediction(victim, clone);
}

} // namespace mt_solver
