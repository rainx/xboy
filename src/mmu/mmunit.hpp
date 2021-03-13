#pragma once

#include "mmu/memory.hpp"

namespace mmu {

class Mmunit : Memory {
public:
  Mmunit();
  virtual ~Mmunit();
  // No copy and move ctor
  Mmunit(const Mmunit &) = delete;
  Mmunit(Mmunit &&) = delete;

  virtual uint8_t get(const uint16_t &address) const override;
  virtual void set(const uint16_t &address, const uint8_t value) override;
};

}; // namespace mmu
