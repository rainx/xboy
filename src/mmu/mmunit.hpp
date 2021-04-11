#pragma once

#include "mmu/cartridge/cartridge.hpp"
#include "mmu/memory.hpp"

#include <memory>
#include <string>

using mmu::cartridge::Cartridge;
using std::string;

namespace mmu {

class Mmunit : Memory {
public:
  Mmunit(std::shared_ptr<Cartridge> cartridge) : cartridge_(cartridge){};
  virtual ~Mmunit();
  // No copy and move ctor
  Mmunit(const Mmunit &) = delete;
  Mmunit(Mmunit &&) = delete;

  static std::shared_ptr<Mmunit> powerUp(const string &rom_path);

  virtual uint8_t get(const uint16_t &address) const override;
  virtual void set(const uint16_t &address, const uint8_t value) override;

protected:
  std::shared_ptr<Cartridge> cartridge_;
};

}; // namespace mmu
