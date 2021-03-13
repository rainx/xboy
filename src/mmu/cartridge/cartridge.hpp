#pragma once

// As the gameboys 16 bit address bus offers only limited space for ROM and RAM
// addressing, many games are using Memory Bank Controllers (MBCs) to expand the
// available address space by bank switching. These MBC chips are located in the
// game cartridge (ie. not in the gameboy itself).
//
// In each cartridge, the required (or preferred) MBC type should be specified
// in the byte at 0147h of the ROM, as described in the cartridge header.
// Several different MBC types are available.
//
// Reference:
//   - http://gbdev.gg8.se/wiki/articles/The_Cartridge_Header
//   - http://gbdev.gg8.se/wiki/articles/Memory_Bank_Controllers

#include "mmu/memory.hpp"
#include <array>
#include <string>

using std::array;
using std::string;

namespace mmu {
namespace cartridge {

/**
 * @brief Cartridge Type
 *   Specifies which Memory Bank Controller (if any) is used in the cartridge,
 * and if further external hardware exists in the cartridge. 00h  ROM ONLY 19h
 * MBC5 01h  MBC1                     1Ah  MBC5+RAM 02h  MBC1+RAM 1Bh
 * MBC5+RAM+BATTERY 03h  MBC1+RAM+BATTERY         1Ch  MBC5+RUMBLE 05h  MBC2 1Dh
 * MBC5+RUMBLE+RAM 06h  MBC2+BATTERY             1Eh  MBC5+RUMBLE+RAM+BATTERY
 *   08h  ROM+RAM                  20h  MBC6
 *   09h  ROM+RAM+BATTERY          22h  MBC7+SENSOR+RUMBLE+RAM+BATTERY
 *   0Bh  MMM01
 *   0Ch  MMM01+RAM
 *   0Dh  MMM01+RAM+BATTERY
 *   0Fh  MBC3+TIMER+BATTERY
 *   10h  MBC3+TIMER+RAM+BATTERY   FCh  POCKET CAMERA
 *   11h  MBC3                     FDh  BANDAI TAMA5
 *   12h  MBC3+RAM                 FEh  HuC3
 *   13h  MBC3+RAM+BATTERY         FFh  HuC1+RAM+BATTERY
 */
enum CartridgeType {
  RomOnly = 0x00,
  MBC1 = 0x01,
  Mbc1WithRam = 0x02,
  Mbc1WithRamAndBattery = 0x01,
  Mbc2 = 0x05,
  Mbc2WithBattery = 0x06,
  RomAndRam = 0x08,
  RomAndRamAndBattery = 0x09,
  Mmm01 = 0x0b,
  Mmm01WithRam = 0x0c,
  Mmm01WithRamAndBattery = 0x0d,
  Mbc3WithTimerAndBattery = 0x0f,
  Mbc3WithTimerAndRamAndBattery = 0x10,
  Mbc3 = 0x11,
  Mbc3WithRam = 0x12,
  Mbc3WithRamAndBattery = 0x13,
  Mbc5 = 0x19,
  Mbc5WithRam = 0x1a,
  Mbc5WithRamAndBattery = 0x1b,
  Mbc5WithRumble = 0x1c,
  Mbc5WithRumbleAndRam = 0x1d,
  Mbc5WithRumbleAndRamAndBattery = 0x1e,
  Mbc6 = 0x20,
  Mbc7WithSensorAndRumbleAndRamAndBattery = 0x22,
  PocketCamera = 0xfc,
  BandaiTama5 = 0xfd,
  HuC3 = 0xfe,
  HuC1WithRamAndBattery = 0xff,
};

template <uint32_t MAX_ROM_SIZE = 0, uint32_t MAX_RAM_SIZE = 0>
class Cartridge : Memory {
public:
  Cartridge(const CartridgeType cartridge_type, const array<MAX_ROM_SIZE> &rom,
            const array<MAX_RAM_SIZE> &ram, const string &rom_path);
  virtual ~Cartridge();
  virtual void save() = 0;
  // Factory to return Correct Cartridge from rom
  static Cartridge powerUp(const string &rom_path);

protected:
  CartridgeType cartridge_type_;
  string rom_path_;
  array<MAX_ROM_SIZE> rom_;
  array<MAX_RAM_SIZE> ram_;
};

}; // namespace cartridge
}; // namespace mmu