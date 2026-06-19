/**
 * @file session.hpp
 * @brief Session token server simulation and hijacking attack via MT19937 cloning.
 */

#pragma once

#include <cstdint>
#include <string>

/**
 * @brief Simulated session token server backed by a weak MT19937 PRNG.
 *
 * Tokens are 32-bit values formatted as 8-character hex strings. The server
 * uses a single global MT19937 instance seeded at construction time; every
 * call to issue_token() advances the PRNG by one step.
 */
struct TokenServer {
  struct MT19937 *rng;

  explicit TokenServer(uint32_t seed);
  ~TokenServer();

  /** @brief Issues one session token to the next connecting client. */
  std::string issue_token();
};

namespace session_attack {

/**
 * @brief Runs the full session hijacking PoC.
 *
 * Simulates a legitimate client that observes 624 tokens, clones the server
 * PRNG state, and then predicts the token issued to the next (victim) client
 * before it is even generated -- demonstrating a complete session hijack.
 *
 * @return 0 on success, non-zero if the predicted token does not match.
 */
int run_session_poc();

} // namespace session_attack
