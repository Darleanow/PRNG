#include <algorithm>

#include <grendizer/grendizer.hpp>
#include <prng/lfsr/lfsr.hpp>
#include <spdlog/spdlog.h>

int main(int argc, char **argv) {
  using namespace spdlog::level;

  int verbose = 0;
  int lfsr_flag = 0;
  gr_opt opts[] = {GR_COUNT('v', "verbose", &verbose,
                            "Enable verbose output (with levels -v -vv)"),
                   GR_FLAG('l', "lfsr", &lfsr_flag, "Enable lfsr PoC"), GR_END};
  gr_spec spec = {"prng_app", "prng_app [options] <input>", opts, NULL};
  gr_rest rest;

  int rc = gr_parse(&spec, argc, argv, &rest, NULL, 0);
  if (rc != GR_OK) {
    spdlog::critical("Couldn't parse arguments.");
    return rc == GR_HELP ? 0 : 1;
  }

  constexpr level_enum levels[] = {warn, info, debug, trace};
  spdlog::set_level(levels[std::clamp(verbose, 0, 3)]);

  if (lfsr_flag) {
    return solver::run_lfsr_poc();
  }

  return 0;
}
