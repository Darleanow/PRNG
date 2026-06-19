/**
 * @file main.cpp
 * @brief Entry point for the PRNG application.
 *
 * Parses CLI arguments via Grendizer and dispatches to the requested PoC.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 * @return 0 on success, non-zero on failure.
 */

#include <algorithm>

#include <grendizer/grendizer.hpp>
#include <prng/lfsr/lfsr.hpp>
#include <prng/mt/mt.hpp>
#include <spdlog/spdlog.h>

int main(int argc, char **argv) {
  using namespace spdlog::level;

  int verbose   = 0;
  int lfsr_flag = 0;
  int mt_flag   = 0;

  gr_opt opts[] = {
    GR_COUNT('v', "verbose", &verbose,   "Verbosity level (-v info, -vv debug, -vvv trace)"),
    GR_FLAG('l',  "lfsr",    &lfsr_flag, "Run LFSR algebraic reconstruction PoC"),
    GR_FLAG('m',  "mt",      &mt_flag,   "Run MT19937 state-cloning PoC"),
    GR_END
  };
  gr_spec spec = {"prng_app", "prng_app [options]", opts, NULL};
  gr_rest rest;

  int rc = gr_parse(&spec, argc, argv, &rest, NULL, 0);
  if (rc != GR_OK) {
    spdlog::critical("Couldn't parse arguments.");
    return rc == GR_HELP ? 0 : 1;
  }

  constexpr level_enum levels[] = {warn, info, debug, trace};
  spdlog::set_level(levels[std::clamp(verbose, 0, 3)]);

  if (lfsr_flag) return solver::run_lfsr_poc();
  if (mt_flag)   return mt_solver::run_mt_poc();

  return 0;
}
