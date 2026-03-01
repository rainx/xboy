#pragma once

#include "./cartridge.hpp"
#include "state/serializable.hpp"
#include <memory>

namespace mmu {
namespace cartridge {

const uint32_t max_mbc1_rom_size = 128 * 1024 * 16; //~ 2M
const uint32_t max_mbc1_ram_size = 8 * 1024 * 4;    // 16k * 4

class Mbc1 : public RomBasedCartridge<max_mbc1_rom_size, max_mbc1_ram_size> {
public:
  Mbc1(const CartridgeType cartridge_type,
       const std::shared_ptr<array<uint8_t, max_mbc1_rom_size>> &rom,
       const BankMode bank_mode, const string &rom_path)
      : RomBasedCartridge<max_mbc1_rom_size, max_mbc1_ram_size>(
            cartridge_type, rom,
            std::make_shared<array<uint8_t, max_mbc1_ram_size>>(), rom_path),
        bank_mode_(bank_mode), bank_(0x01), ram_enabled_(false){};

  virtual uint8_t get(const uint16_t &address) const override {
    if (address >= 0 && address <= 0x3fff) {
      return (*rom_)[address];
    } else if (address >= 0x4000 && address <= 0x7fff) {
      uint32_t mapped_address = getRomBank() * 0x4000 + address - 0x4000;
      return (*rom_)[mapped_address];
    } else if (address >= 0xa000 && address <= 0xbfff) {
      if (ram_enabled_) {
        uint32_t mapped_address = getRamBank() * 0x2000 + address - 0xa000;
        return (*ram_)[mapped_address];
      } else {
        return 0x00;
      }
    }

    return 0x00;
  };

  virtual void set(const uint16_t &address, const uint8_t value) override {
    if (address >= 0xa000 && address <= 0xbfff) {
      if (ram_enabled_) {
        uint32_t mapped_address = getRamBank() * 0x2000 + address - 0xa000;
        (*ram_)[mapped_address] = value;
      }
    } else if (address >= 0x0000 && address <= 0x1fff) {
      ram_enabled_ = (value & 0x0f) == 0x0a;
      if (!ram_enabled_) {
        save();
      }
    } else if (address >= 0x2000 && address <= 0x3fff) {
      uint32_t bank_number = value & 0x1f;
      if (bank_number == 0x00) {
        bank_number = 0x01;
      }
      bank_ = (bank_ & 0x60) | bank_number;
    } else if (address >= 0x4000 && address <= 0x5fff) {
      auto n = value & 0x03;
      bank_ = bank_ & 0x9f | (n << 5);
    } else if (address >= 0x6000 && address <= 0x7fff) {
      if (value == 0x00) {
        bank_mode_ = Rom;
      } else if (value == 0x01) {
        bank_mode_ = Ram;
      }
      // Invalid value
    }
  }

  void serialize(std::vector<uint8_t> &buf) const override {
    state::write_u8(buf, static_cast<uint8_t>(bank_mode_));
    state::write_u8(buf, bank_);
    state::write_bool(buf, ram_enabled_);
    state::write_bytes(buf, ram_->data(), ram_->size());
  }

  void deserialize(const uint8_t *data, size_t &pos) override {
    bank_mode_ = static_cast<BankMode>(state::read_u8(data, pos));
    bank_ = state::read_u8(data, pos);
    ram_enabled_ = state::read_bool(data, pos);
    state::read_bytes(data, pos, ram_->data(), ram_->size());
  }

protected:
  BankMode bank_mode_;
  uint8_t bank_;

private:
  uint8_t getRomBank() const {
    if (bank_mode_ == Rom) {
      return bank_ & 0x7f;
    } else { // bank_mode_ == Ram
      return bank_ & 0x1f;
    };
  };

  uint8_t getRamBank() const {
    if (bank_mode_ == Rom) {
      return 0x00;
    } else { // bank_mode_ == Rm
      return (bank_ & 0x60) >> 5;
    }
  };

  bool ram_enabled_ = false;
};

}; // namespace cartridge
}; // namespace mmu
