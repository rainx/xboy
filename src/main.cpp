#include <iostream>
#include "mmu/linear-memory.hpp"

using namespace mmu;

int main(int, char**) {
    LinearMemory<100> memory;
    std::cout << "Hello, world!\n" << static_cast<int>(memory.get(0)) << "\n";
}
