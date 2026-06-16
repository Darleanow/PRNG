#include <iostream>
#include <prng/lfsr/lfsr.hpp>

int main(void) {

  LFSR lfsr;
  lfsr.val = 1;

  for (size_t i = 0; i < 15; i++) {
    auto out = step(lfsr);
    std::cout << out << " ";
  }

  return 0;
}
