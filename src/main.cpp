#include <grendizer/grendizer.hpp>
#include <prng/lfsr/lfsr.hpp>
#include <spdlog/spdlog.h>

int main(int argc, char **argv) {
  using namespace spdlog::level;

  int verbose = 0;
  int lfsr_flag = 0;
  gr_opt opts[] = {GR_COUNT('v', "verbose", &verbose,
                            "Enable verbose output (with levels -v -vv -vvv)"),
                   GR_FLAG('l', "lfsr", &lfsr_flag, "Enable lfsr PoC"), GR_END};
  gr_spec spec = {"prng_app", "prng_app [options] <input>", opts, NULL};
  gr_rest rest;

  int rc = gr_parse(&spec, argc, argv, &rest, NULL, 0);
  if (rc != GR_OK) {
    spdlog::critical("Couldn't parse arguments.");
    return rc == GR_HELP ? 0 : 1;
  }

  constexpr level_enum levels[] = {warn, info, debug, trace};
  int idx = std::clamp(verbose, 0, 3);
  spdlog::set_level(levels[idx]);

  if (lfsr_flag) {
    spdlog::info("Beginning PoC of lfsr break with GF(2)");

    LFSR victim;
    victim.val = 0xDEADBEEF;
    spdlog::debug("Initial state={}", victim.val);

    constexpr size_t N = 32;
    uint8_t observed[2 * N];

    for (size_t n = 0; n < 2 * N; n++) {
      unsigned out = step(victim);
      observed[n] = out;
      spdlog::debug("step {}: state={}, out={}", n, victim.val, out);
    }
    spdlog::info("Done observing !");
  }

  return 0;
}
