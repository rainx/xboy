#pragma once
#include "mmu/memory.hpp"
#include <array>

namespace mmu {
template <uint16_t MEMORY_SIZE> class LinearMemory : public Memory {
public:
  LinearMemory();
  virtual ~LinearMemory();
  // Copy constructor
  LinearMemory(const LinearMemory &memory) = delete;
  // Move constructor
  LinearMemory(LinearMemory &&memory) = delete;

  uint8_t get(const uint16_t &address) const;
  virtual void set(const uint16_t &address, const uint8_t value);

protected:
  std::array<uint8_t, MEMORY_SIZE> memory_;

private:
  /// Reset the memory to be zero filled
  void reset();
};
}; // namespace mmu
