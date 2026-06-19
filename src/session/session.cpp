/**
 * @file session.cpp
 * @brief Session token server simulation and MT19937-based hijacking attack.
 */

#include "prng/session/session.hpp"
#include "prng/mt/mt.hpp"
#include <spdlog/spdlog.h>
#include <format>
#include <cstdlib>
#include <ctime>

TokenServer::TokenServer(uint32_t seed) : rng(new MT19937) {
  mt::seed(*rng, seed);
}

TokenServer::~TokenServer() {
  delete rng;
}

std::string TokenServer::issue_token() {
  return std::format("{:08x}", mt::next(*rng));
}

namespace session_attack {

/**
 * @brief Parses a hex token string back into its raw 32-bit value.
 *
 * @param token  8-character lowercase hex string produced by TokenServer.
 * @return       Parsed value.
 */
static uint32_t parse_token(const std::string &token) {
  return static_cast<uint32_t>(std::stoul(token, nullptr, 16));
}

/**
 * @brief Observes @p n tokens from the server and stores their raw values.
 *
 * Models a legitimate client making repeated requests and recording every
 * token it receives.
 *
 * @param server    Running token server.
 * @param out       Output buffer of size @p n.
 * @param n         Number of tokens to collect.
 */
static void collect_tokens(TokenServer &server, uint32_t *out, int n) {
  for (int i = 0; i < n; i++) {
    std::string tok = server.issue_token();
    out[i] = parse_token(tok);
    spdlog::debug("observed [{:3}]: {}", i, tok);
  }
}

/**
 * @brief Clones the server PRNG state from @p n observed token values.
 *
 * Applies mt_solver::untemper to each observed value to recover the raw
 * MT19937 state word, then sets the clone index to N so the next twist
 * fires at the same point as the server's next generation.
 *
 * @param observed  Raw token values collected from the server.
 * @param n         Number of observed values (must be 624).
 * @param clone     MT19937 instance to overwrite with the recovered state.
 */
static void clone_state(uint32_t *observed, int n, MT19937 &clone) {
  for (int i = 0; i < n; i++)
    clone.state[i] = mt_solver::untemper(observed[i]);
  clone.index = n;
}

/**
 * @brief Predicts the next token the server will issue without querying it.
 *
 * @param clone  Cloned generator synchronised with the server state.
 * @return       Predicted token string.
 */
static std::string predict_next(MT19937 &clone) {
  return std::format("{:08x}", mt::next(clone));
}

int run_session_poc() {
  spdlog::info("=== Session hijacking PoC ===");

  // Server uses a time-based seed, as a real weak server would.
  const uint32_t seed = static_cast<uint32_t>(std::time(nullptr));
  TokenServer server(seed);
  spdlog::info("Server started (seed unknown to attacker)");

  // Step 1: legitimate client collects 624 tokens (one full MT state block).
  constexpr int N = 624;
  uint32_t observed[N];
  collect_tokens(server, observed, N);
  spdlog::info("Step 1: attacker collected {} tokens", N);

  // Step 2: clone the internal PRNG state.
  MT19937 clone;
  clone_state(observed, N, clone);
  spdlog::info("Step 2: PRNG state fully reconstructed");

  // Step 3: predict the token that will be issued to the next (victim) client.
  std::string predicted = predict_next(clone);
  spdlog::info("Step 3: attacker predicts next token = {}", predicted);

  // Step 4: victim connects. server issues their token.
  std::string victim_token = server.issue_token();
  spdlog::info("Step 4: server issued victim token  = {}", victim_token);

  // Step 5: compare.
  if (predicted != victim_token) {
    spdlog::error("Hijack failed: predicted={} actual={}", predicted, victim_token);
    return 1;
  }

  spdlog::info("HIJACK SUCCESSFUL: attacker can impersonate the victim");
  spdlog::info("  predicted token : {}", predicted);
  spdlog::info("  victim token    : {}", victim_token);
  spdlog::info("  => attacker sends token '{}' and gets victim's session", predicted);
  return 0;
}

} // namespace session_attack
