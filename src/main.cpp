#include "mmu/linear-memory.hpp"
#include <iostream>

using namespace mmu;

int main(int, char **) {
  LinearMemory<100> memory;
  std::cout << "Hello, world!\n" << static_cast<int>(memory.get(0)) << "\n";
}
