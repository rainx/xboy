#pragma once

#include "./cartridge.hpp"

namespace mmu {
namespace cartridge {

constexpr uint32_t rom_only_cartrige_size = 32 * 1024;

class RomOnlyCartridge : public RomBasedCartridge<rom_only_cartrige_size, 0> {
public:
  RomOnlyCartridge(
      const std::shared_ptr<array<uint8_t, rom_only_cartrige_size>> &rom,
      const string &rom_path)
      : RomBasedCartridge<rom_only_cartrige_size, 0>(
            RomOnly, rom, std::make_shared<array<uint8_t, 0>>(), rom_path){};
};

} // namespace cartridge
} // namespace mmu