#include <iostream>
#include <prng/lfsr/lfsr.hpp>

int main(void) {

  LFSR lfsr;
  lfsr.val = 1;
  uint64_t period{0};

  uint32_t start = lfsr.val;

  do {
    step(lfsr);
    period++;
    if (period > 5000000000ULL) {
      std::cout << "boucle infinie, val actuel = " << lfsr.val << "\n";
      return 1;
    }
  } while (lfsr.val != start);

  std::cout << period << " : " << 4294967295;
  return 0;
}
