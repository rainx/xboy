
#include <array>
#include <memory>
#include <string>

#include "./cartridge.hpp"
#include "./rom-only-cartridge.hpp"

#include <memory>

using namespace mmu::cartridge;

using std::array;
using std::string;

std::shared_ptr<Cartridge>
mmu::cartridge::Cartridge::powerUp(const string &rom_path) {
  const auto rom = std::make_shared<array<uint8_t, 32 * 1024>>();
  const auto ram = std::make_shared<array<uint8_t, 0>>();

  return std::make_shared<RomOnlyCartridge>(RomOnly, rom, ram, rom_path);
}

template class mmu::cartridge::RomBasedCartridge<32 * 1024, 0>;