#pragma once

#include "mmu/cartridge/cartridge.hpp"
#include "mmu/memory.hpp"

#include <array>
#include <memory>
#include <string>

using mmu::cartridge::Cartridge;
using std::string;

namespace mmu {

class Mmunit : public Memory {
public:
  Mmunit(std::shared_ptr<Cartridge> cartridge);
  virtual ~Mmunit();
  Mmunit(const Mmunit &) = delete;
  Mmunit(Mmunit &&) = delete;

  static std::shared_ptr<Mmunit> powerUp(const string &rom_path);

  virtual uint8_t get(const uint16_t &address) const override;
  virtual void set(const uint16_t &address, const uint8_t value) override;

  // Serial port output capture (for Blargg tests)
  std::string getSerialOutput() const { return serial_output_; }

private:
  std::shared_ptr<Cartridge> cartridge_;

  // Internal memory regions
  std::array<uint8_t, 0x2000> vram_{};   // 8KB Video RAM (0x8000-0x9FFF)
  std::array<uint8_t, 0x2000> wram_{};   // 8KB Work RAM  (0xC000-0xDFFF)
  std::array<uint8_t, 0xA0> oam_{};      // 160B Sprite Attribute Table (0xFE00-0xFE9F)
  std::array<uint8_t, 0x80> io_{};       // 128B I/O Registers (0xFF00-0xFF7F)
  std::array<uint8_t, 0x7F> hram_{};     // 127B High RAM (0xFF80-0xFFFE)
  uint8_t ie_ = 0;                        // Interrupt Enable Register (0xFFFF)

  // Serial output buffer (for test ROM output capture)
  mutable std::string serial_output_;
};

}; // namespace mmu
