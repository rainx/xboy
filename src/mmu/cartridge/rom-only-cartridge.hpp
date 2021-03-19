#pragma once

#include "./cartridge.hpp"

namespace mmu {
namespace cartridge {

class RomOnlyCartridge : public RomBasedCartridge<32 * 1024, 0> {
    public: 
        RomOnlyCartridge(const std::shared_ptr<array<uint8_t, 32 * 1024>>& rom, const string& rom_path):
            RomBasedCartridge<32 * 1024, 0> (
                RomOnly,
                rom,
                std::make_shared<array<uint8_t, 0>>(),
                rom_path
            )
          {};

};

} // namespace cartridge
} // namespace mmu