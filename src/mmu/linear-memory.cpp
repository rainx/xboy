#include "mmu/linear-memory.hpp"

using namespace mmu;

template <uint16_t MEMORY_SIZE> mmu::LinearMemory<MEMORY_SIZE>::LinearMemory() {
  reset();
}

template <uint16_t MEMORY_SIZE>
mmu::LinearMemory<MEMORY_SIZE>::~LinearMemory() {}

template <uint16_t MEMORY_SIZE>
uint8_t mmu::LinearMemory<MEMORY_SIZE>::get(const uint16_t &address) const {
  return memory_.at(address);
}

template <uint16_t MEMORY_SIZE>
void mmu::LinearMemory<MEMORY_SIZE>::set(const uint16_t &address,
                                         const uint8_t value) {
  memory_[address] = value;
}

// Protected
template <uint16_t MEMORY_SIZE> void mmu::LinearMemory<MEMORY_SIZE>::reset() {
  memory_.fill(0);
}

// used template should be declared here to avoid buliding error
// for google test
template class mmu::LinearMemory<100>;
