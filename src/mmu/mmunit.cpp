
#include <memory>

#include "cartridge/cartridge.hpp"
#include "mmu/mmunit.hpp"

using namespace mmu;
using mmu::cartridge::Cartridge;

uint8_t mmu::Mmunit::get(const uint16_t &address) const {
  if (address >= 0x0000 && address <= 0x7fff) {
    return cartridge_->get(address);
  }

  return 0;
}

void mmu::Mmunit::set(const uint16_t &address, const uint8_t value) {}

static std::shared_ptr<Mmunit> powerUp(const string &rom_path) {
  std::shared_ptr<Cartridge> cartridge = Cartridge::powerUp(rom_path);
  return std::make_shared<Mmunit>(cartridge);
}