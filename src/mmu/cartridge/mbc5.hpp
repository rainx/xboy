#pragma once

#include "./cartridge.hpp"
#include "state/serializable.hpp"
#include <memory>

namespace mmu {
namespace cartridge {

const uint32_t max_mbc5_rom_size = 512 * 16 * 1024; // 8MB (512 banks x 16KB)
const uint32_t max_mbc5_ram_size = 16 * 8 * 1024;   // 128KB (16 banks x 8KB)

class Mbc5 : public RomBasedCartridge<max_mbc5_rom_size, max_mbc5_ram_size> {
public:
  Mbc5(const CartridgeType cartridge_type,
       const std::shared_ptr<array<uint8_t, max_mbc5_rom_size>> &rom,
       const string &rom_path)
      : RomBasedCartridge<max_mbc5_rom_size, max_mbc5_ram_size>(
            cartridge_type, rom,
            std::make_shared<array<uint8_t, max_mbc5_ram_size>>(), rom_path),
        rom_bank_(0x01), ram_bank_(0x00), ram_enabled_(false),
        has_rumble_(cartridge_type == Mbc5WithRumble ||
                    cartridge_type == Mbc5WithRumbleAndRam ||
                    cartridge_type == Mbc5WithRumbleAndRamAndBattery) {
    if (hasBattery()) {
      loadRam();
    }
  };

  virtual uint8_t get(const uint16_t &address) const override {
    if (address <= 0x3fff) {
      // Bank 0 — always mapped
      return (*rom_)[address];
    } else if (address >= 0x4000 && address <= 0x7fff) {
      // Switchable ROM bank
      uint32_t mapped = static_cast<uint32_t>(rom_bank_) * 0x4000 +
                        (address - 0x4000);
      return (*rom_)[mapped];
    } else if (address >= 0xa000 && address <= 0xbfff) {
      if (ram_enabled_) {
        uint32_t mapped = static_cast<uint32_t>(ram_bank_) * 0x2000 +
                          (address - 0xa000);
        return (*ram_)[mapped];
      }
      return 0xff;
    }
    return 0xff;
  };

  virtual void set(const uint16_t &address, const uint8_t value) override {
    if (address <= 0x1fff) {
      // RAM enable
      ram_enabled_ = (value & 0x0f) == 0x0a;
      if (!ram_enabled_ && hasBattery()) {
        save();
      }
    } else if (address >= 0x2000 && address <= 0x2fff) {
      // ROM bank — low 8 bits
      rom_bank_ = (rom_bank_ & 0x100) | value;
    } else if (address >= 0x3000 && address <= 0x3fff) {
      // ROM bank — bit 8
      rom_bank_ = (rom_bank_ & 0x0ff) | ((value & 0x01) << 8);
    } else if (address >= 0x4000 && address <= 0x5fff) {
      // RAM bank (4 bits; with rumble: bit 3 = motor, bits 0-2 = bank)
      if (has_rumble_) {
        ram_bank_ = value & 0x07;
      } else {
        ram_bank_ = value & 0x0f;
      }
    } else if (address >= 0xa000 && address <= 0xbfff) {
      // RAM write
      if (ram_enabled_) {
        uint32_t mapped = static_cast<uint32_t>(ram_bank_) * 0x2000 +
                          (address - 0xa000);
        (*ram_)[mapped] = value;
      }
    }
  }

  void serialize(std::vector<uint8_t> &buf) const override {
    state::write_u16(buf, rom_bank_);
    state::write_u8(buf, ram_bank_);
    state::write_bool(buf, ram_enabled_);
    state::write_bytes(buf, ram_->data(), ram_->size());
  }

  void deserialize(const uint8_t *data, size_t &pos) override {
    rom_bank_ = state::read_u16(data, pos);
    ram_bank_ = state::read_u8(data, pos);
    ram_enabled_ = state::read_bool(data, pos);
    state::read_bytes(data, pos, ram_->data(), ram_->size());
  }

private:
  bool hasBattery() const {
    return cartridge_type_ == Mbc5WithRamAndBattery ||
           cartridge_type_ == Mbc5WithRumbleAndRamAndBattery;
  }

  uint16_t rom_bank_;
  uint8_t ram_bank_;
  bool ram_enabled_;
  bool has_rumble_;
};

}; // namespace cartridge
}; // namespace mmu
