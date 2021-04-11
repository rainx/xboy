
#include <array>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>

#include "./cartridge.hpp"
#include "./mbc1.hpp"
#include "./rom-only-cartridge.hpp"
#include "kaitai-struct-gen/cartridge-header.h"

using namespace mmu::cartridge;

using std::array;
using std::ifstream;
using std::string;

std::shared_ptr<Cartridge>
mmu::cartridge::Cartridge::powerUp(const string &rom_path) {

  ifstream rom_ifstream(rom_path, ifstream::binary);
  kaitai::kstream ks_rom(&rom_ifstream);
  gen::cartridge_header_t cartrigde_header = gen::cartridge_header_t(&ks_rom);

  CartridgeType cartridge_type =
      static_cast<CartridgeType>(cartrigde_header.cartridge_type());
  rom_ifstream.seekg(0);
  uint8_t one_byte_to_read;
  uint32_t index_of_reader = 0;
  if (cartridge_type == RomOnly) {
    const auto rom = std::make_shared<array<uint8_t, rom_only_cartrige_size>>();
    // max size is 32 * 1024
    rom_ifstream.read((char *)rom->data(), rom->size());
    return std::make_shared<RomOnlyCartridge>(rom, rom_path);
  } else if (cartridge_type == MBC1 || cartridge_type == Mbc1WithRam ||
             cartridge_type == Mbc1WithRamAndBattery) {
    const auto rom = std::make_shared<array<uint8_t, max_mbc1_rom_size>>();
    rom_ifstream.read((char *)rom->data(), rom->size());
    return std::make_shared<Mbc1>(cartridge_type, rom,
                                  cartridge_type == MBC1 ? Rom : Ram, rom_path);
  } else {
    std::stringstream unsupportted_type;
    unsupportted_type << "Not yet support the type " << std::hex
                      << cartridge_type;
    throw std::runtime_error(unsupportted_type.str());
  }
}

template class mmu::cartridge::RomBasedCartridge<rom_only_cartrige_size, 0>;
template class mmu::cartridge::RomBasedCartridge<max_mbc1_rom_size,
                                                 max_mbc1_ram_size>;