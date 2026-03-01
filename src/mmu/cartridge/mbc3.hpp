#pragma once

#include "./cartridge.hpp"
#include <chrono>
#include <cstdint>
#include <memory>

namespace mmu {
namespace cartridge {

const uint32_t max_mbc3_rom_size = 128 * 16 * 1024; // 2MB (128 banks x 16KB)
const uint32_t max_mbc3_ram_size = 4 * 8 * 1024;    // 32KB (4 banks x 8KB)

struct RtcRegisters {
  uint8_t seconds = 0;  // 0x08
  uint8_t minutes = 0;  // 0x09
  uint8_t hours = 0;    // 0x0A
  uint8_t days_low = 0; // 0x0B — lower 8 bits of day counter
  uint8_t days_high = 0; // 0x0C — bit 0: day counter MSB
                          //        bit 6: halt flag
                          //        bit 7: day carry flag
};

class Mbc3 : public RomBasedCartridge<max_mbc3_rom_size, max_mbc3_ram_size> {
public:
  Mbc3(const CartridgeType cartridge_type,
       const std::shared_ptr<array<uint8_t, max_mbc3_rom_size>> &rom,
       const string &rom_path)
      : RomBasedCartridge<max_mbc3_rom_size, max_mbc3_ram_size>(
            cartridge_type, rom,
            std::make_shared<array<uint8_t, max_mbc3_ram_size>>(), rom_path),
        rom_bank_(0x01), ram_rtc_select_(0x00), ram_enabled_(false),
        latch_ready_(false), rtc_base_seconds_(0) {
    rtc_start_time_ = std::chrono::system_clock::now();
    if (hasBattery()) {
      loadRam();
      loadRtc();
    }
  };

  virtual uint8_t get(const uint16_t &address) const override {
    if (address <= 0x3fff) {
      return (*rom_)[address];
    } else if (address >= 0x4000 && address <= 0x7fff) {
      uint32_t mapped = static_cast<uint32_t>(rom_bank_) * 0x4000 +
                        (address - 0x4000);
      return (*rom_)[mapped];
    } else if (address >= 0xa000 && address <= 0xbfff) {
      if (!ram_enabled_) {
        return 0xff;
      }
      if (ram_rtc_select_ <= 0x03) {
        // RAM bank access
        uint32_t mapped = static_cast<uint32_t>(ram_rtc_select_) * 0x2000 +
                          (address - 0xa000);
        return (*ram_)[mapped];
      } else if (ram_rtc_select_ >= 0x08 && ram_rtc_select_ <= 0x0c) {
        return readRtcRegister(ram_rtc_select_);
      }
      return 0xff;
    }
    return 0xff;
  };

  virtual void set(const uint16_t &address, const uint8_t value) override {
    if (address <= 0x1fff) {
      // RAM & RTC enable
      ram_enabled_ = (value & 0x0f) == 0x0a;
      if (!ram_enabled_ && hasBattery()) {
        save();
        saveRtc();
      }
    } else if (address >= 0x2000 && address <= 0x3fff) {
      // ROM bank — 7 bits, 0 maps to 1
      rom_bank_ = value & 0x7f;
      if (rom_bank_ == 0x00) {
        rom_bank_ = 0x01;
      }
    } else if (address >= 0x4000 && address <= 0x5fff) {
      // RAM bank or RTC register select
      ram_rtc_select_ = value;
    } else if (address >= 0x6000 && address <= 0x7fff) {
      // RTC latch — write 0x00 then 0x01
      if (value == 0x00) {
        latch_ready_ = true;
      } else if (value == 0x01 && latch_ready_) {
        latchRtc();
        latch_ready_ = false;
      } else {
        latch_ready_ = false;
      }
    } else if (address >= 0xa000 && address <= 0xbfff) {
      if (!ram_enabled_) {
        return;
      }
      if (ram_rtc_select_ <= 0x03) {
        uint32_t mapped = static_cast<uint32_t>(ram_rtc_select_) * 0x2000 +
                          (address - 0xa000);
        (*ram_)[mapped] = value;
      } else if (ram_rtc_select_ >= 0x08 && ram_rtc_select_ <= 0x0c) {
        writeRtcRegister(ram_rtc_select_, value);
      }
    }
  }

  virtual void save() override {
    RomBasedCartridge::save();
    if (hasRtc()) {
      saveRtc();
    }
  }

  // Exposed for testing
  const RtcRegisters &getLatchedRtc() const { return latched_rtc_; }

  void setRtcStartTime(std::chrono::system_clock::time_point tp) {
    rtc_start_time_ = tp;
  }

  void setRtcBaseSeconds(int64_t s) { rtc_base_seconds_ = s; }

  RtcRegisters &getRtcRegisters() { return rtc_; }

private:
  bool hasBattery() const {
    return cartridge_type_ == Mbc3WithRamAndBattery ||
           cartridge_type_ == Mbc3WithTimerAndBattery ||
           cartridge_type_ == Mbc3WithTimerAndRamAndBattery;
  }

  bool hasRtc() const {
    return cartridge_type_ == Mbc3WithTimerAndBattery ||
           cartridge_type_ == Mbc3WithTimerAndRamAndBattery;
  }

  bool isHalted() const { return (rtc_.days_high & 0x40) != 0; }

  int64_t computeCurrentSeconds() const {
    int64_t total = rtc_base_seconds_;
    if (!isHalted()) {
      auto now = std::chrono::system_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                         now - rtc_start_time_)
                         .count();
      total += elapsed;
    }
    return total;
  }

  void latchRtc() {
    int64_t total_seconds = computeCurrentSeconds();

    latched_rtc_.seconds = total_seconds % 60;
    latched_rtc_.minutes = (total_seconds / 60) % 60;
    latched_rtc_.hours = (total_seconds / 3600) % 24;

    int32_t days = static_cast<int32_t>(total_seconds / 86400);
    latched_rtc_.days_low = days & 0xff;

    uint8_t dh = rtc_.days_high & 0xc0; // preserve halt & carry flags
    dh |= ((days >> 8) & 0x01);         // day counter bit 8
    if (days > 511) {
      dh |= 0x80; // carry flag
    }
    latched_rtc_.days_high = dh;
  }

  uint8_t readRtcRegister(uint8_t reg) const {
    switch (reg) {
    case 0x08:
      return latched_rtc_.seconds;
    case 0x09:
      return latched_rtc_.minutes;
    case 0x0a:
      return latched_rtc_.hours;
    case 0x0b:
      return latched_rtc_.days_low;
    case 0x0c:
      return latched_rtc_.days_high;
    default:
      return 0xff;
    }
  }

  void writeRtcRegister(uint8_t reg, uint8_t value) {
    // Writing to an RTC register resets the base counter to match the
    // new register values, and restarts the clock from now.
    flushToBaseSeconds();

    switch (reg) {
    case 0x08:
      rtc_.seconds = value & 0x3f;
      break;
    case 0x09:
      rtc_.minutes = value & 0x3f;
      break;
    case 0x0a:
      rtc_.hours = value & 0x1f;
      break;
    case 0x0b:
      rtc_.days_low = value;
      break;
    case 0x0c:
      rtc_.days_high = value;
      break;
    }
    rebuildBaseFromRegisters();
  }

  void flushToBaseSeconds() {
    rtc_base_seconds_ = computeCurrentSeconds();
    rtc_start_time_ = std::chrono::system_clock::now();
  }

  void rebuildBaseFromRegisters() {
    int32_t days = rtc_.days_low | ((rtc_.days_high & 0x01) << 8);
    rtc_base_seconds_ = rtc_.seconds + rtc_.minutes * 60 +
                        rtc_.hours * 3600 +
                        static_cast<int64_t>(days) * 86400;
    rtc_start_time_ = std::chrono::system_clock::now();
  }

  void saveRtc() {
    ofstream rtc_stream(rom_path_ + ".rtc", ofstream::binary | ofstream::out);
    if (!rtc_stream.good()) {
      return;
    }
    int64_t base = computeCurrentSeconds();
    auto unix_ts = std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count();
    rtc_stream.write((char *)&base, sizeof(base));
    rtc_stream.write((char *)&unix_ts, sizeof(unix_ts));
    rtc_stream.write((char *)&latched_rtc_, sizeof(latched_rtc_));
  }

  void loadRtc() {
    ifstream rtc_stream(rom_path_ + ".rtc", ifstream::binary);
    if (!rtc_stream.good()) {
      return;
    }
    int64_t saved_base;
    int64_t saved_unix_ts;
    rtc_stream.read((char *)&saved_base, sizeof(saved_base));
    rtc_stream.read((char *)&saved_unix_ts, sizeof(saved_unix_ts));
    rtc_stream.read((char *)&latched_rtc_, sizeof(latched_rtc_));

    // Reconstruct: add real elapsed time since save
    auto now_unix = std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();
    int64_t elapsed = now_unix - saved_unix_ts;
    rtc_base_seconds_ = saved_base + (elapsed > 0 ? elapsed : 0);
    rtc_start_time_ = std::chrono::system_clock::now();
  }

  uint8_t rom_bank_;
  uint8_t ram_rtc_select_;
  bool ram_enabled_;
  bool latch_ready_;

  // RTC state
  RtcRegisters rtc_;
  RtcRegisters latched_rtc_;
  int64_t rtc_base_seconds_;
  std::chrono::system_clock::time_point rtc_start_time_;
};

}; // namespace cartridge
}; // namespace mmu
