#pragma once

#include "./cartridge.hpp"

namespace mmu {
namespace cartridge {

class RomOnlyCartridge : RomBasedCartridge<32 * 1024, 0> {};

} // namespace cartridge
} // namespace mmu