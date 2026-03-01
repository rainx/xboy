
#include <memory>

#include "cartridge/cartridge.hpp"
#include "mmu/mmunit.hpp"

using namespace mmu;
using mmu::cartridge::Cartridge;

mmu::Mmunit::Mmunit(std::shared_ptr<Cartridge> cartridge)
    : cartridge_(cartridge) {}

mmu::Mmunit::~Mmunit() {}

uint8_t mmu::Mmunit::get(const uint16_t &address) const {
  // 0x0000-0x7FFF: ROM (via cartridge)
  if (address <= 0x7FFF) {
    return cartridge_->get(address);
  }
  // 0x8000-0x9FFF: Video RAM
  else if (address >= 0x8000 && address <= 0x9FFF) {
    return vram_[address - 0x8000];
  }
  // 0xA000-0xBFFF: External RAM (via cartridge)
  else if (address >= 0xA000 && address <= 0xBFFF) {
    return cartridge_->get(address);
  }
  // 0xC000-0xDFFF: Work RAM
  else if (address >= 0xC000 && address <= 0xDFFF) {
    return wram_[address - 0xC000];
  }
  // 0xE000-0xFDFF: Echo RAM (mirrors C000-DDFF)
  else if (address >= 0xE000 && address <= 0xFDFF) {
    return wram_[address - 0xE000];
  }
  // 0xFE00-0xFE9F: OAM
  else if (address >= 0xFE00 && address <= 0xFE9F) {
    return oam_[address - 0xFE00];
  }
  // 0xFEA0-0xFEFF: Not usable
  else if (address >= 0xFEA0 && address <= 0xFEFF) {
    return 0xFF;
  }
  // 0xFF00-0xFF7F: I/O Registers
  else if (address >= 0xFF00 && address <= 0xFF7F) {
    return io_[address - 0xFF00];
  }
  // 0xFF80-0xFFFE: High RAM
  else if (address >= 0xFF80 && address <= 0xFFFE) {
    return hram_[address - 0xFF80];
  }
  // 0xFFFF: Interrupt Enable Register
  else if (address == 0xFFFF) {
    return ie_;
  }

  return 0xFF;
}

void mmu::Mmunit::set(const uint16_t &address, const uint8_t value) {
  // 0x0000-0x7FFF: ROM area (writes go to cartridge for bank switching)
  if (address <= 0x7FFF) {
    cartridge_->set(address, value);
  }
  // 0x8000-0x9FFF: Video RAM
  else if (address >= 0x8000 && address <= 0x9FFF) {
    vram_[address - 0x8000] = value;
  }
  // 0xA000-0xBFFF: External RAM (via cartridge)
  else if (address >= 0xA000 && address <= 0xBFFF) {
    cartridge_->set(address, value);
  }
  // 0xC000-0xDFFF: Work RAM
  else if (address >= 0xC000 && address <= 0xDFFF) {
    wram_[address - 0xC000] = value;
  }
  // 0xE000-0xFDFF: Echo RAM (mirrors C000-DDFF)
  else if (address >= 0xE000 && address <= 0xFDFF) {
    wram_[address - 0xE000] = value;
  }
  // 0xFE00-0xFE9F: OAM
  else if (address >= 0xFE00 && address <= 0xFE9F) {
    oam_[address - 0xFE00] = value;
  }
  // 0xFEA0-0xFEFF: Not usable (writes ignored)
  else if (address >= 0xFEA0 && address <= 0xFEFF) {
    // Ignored
  }
  // 0xFF00-0xFF7F: I/O Registers
  else if (address >= 0xFF00 && address <= 0xFF7F) {
    // DIV: any write resets to 0
    if (address == 0xFF04) {
      io_[0x04] = 0;
      return;
    }

    io_[address - 0xFF00] = value;

    // OAM DMA transfer: copy 160 bytes from (value << 8) to OAM
    if (address == 0xFF46) {
      uint16_t src = static_cast<uint16_t>(value) << 8;
      for (uint16_t i = 0; i < 0xA0; i++) {
        oam_[i] = get(src + i);
      }
    }

    // Serial transfer: capture output when SC triggers transfer
    if (address == 0xFF02 && value == 0x81) {
      serial_output_ += static_cast<char>(io_[0x01]); // SB = 0xFF01
    }
  }
  // 0xFF80-0xFFFE: High RAM
  else if (address >= 0xFF80 && address <= 0xFFFE) {
    hram_[address - 0xFF80] = value;
  }
  // 0xFFFF: Interrupt Enable Register
  else if (address == 0xFFFF) {
    ie_ = value;
  }
}

std::shared_ptr<Mmunit> mmu::Mmunit::powerUp(const string &rom_path) {
  std::shared_ptr<Cartridge> cartridge = Cartridge::powerUp(rom_path);
  return std::make_shared<Mmunit>(cartridge);
}
