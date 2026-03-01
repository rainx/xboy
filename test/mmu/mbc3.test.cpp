
#include "gtest/gtest.h"

#include "mmu/cartridge/mbc3.hpp"
#include <chrono>

namespace mmu {
namespace cartridge {

// Type alias to disambiguate from the CartridgeType enum value
using Mbc3Cart = class Mbc3;

class Mbc3Test : public ::testing::Test {
protected:
  void SetUp() override {
    rom_ = std::make_shared<array<uint8_t, max_mbc3_rom_size>>();
    rom_->fill(0);

    // Write distinct values into each bank's first byte
    for (int bank = 0; bank < 128; bank++) {
      (*rom_)[bank * 0x4000] = bank & 0xff;
    }

    cart_ = std::make_shared<Mbc3Cart>(Mbc3WithTimerAndRamAndBattery, rom_,
                                       "/tmp/mbc3_test");
  }

  std::shared_ptr<array<uint8_t, max_mbc3_rom_size>> rom_;
  std::shared_ptr<Mbc3Cart> cart_;
};

// ===== ROM Banking =====

TEST_F(Mbc3Test, ReadFromBank0) {
  EXPECT_EQ(cart_->get(0x0000), 0x00);
}

TEST_F(Mbc3Test, DefaultRomBankIs1) {
  EXPECT_EQ(cart_->get(0x4000), 0x01);
}

TEST_F(Mbc3Test, RomBankSwitching) {
  cart_->set(0x2000, 0x05);
  EXPECT_EQ(cart_->get(0x4000), 0x05);
}

TEST_F(Mbc3Test, RomBank0MapsTo1) {
  // MBC3 corrects bank 0 to bank 1
  cart_->set(0x2000, 0x00);
  EXPECT_EQ(cart_->get(0x4000), 0x01);
}

TEST_F(Mbc3Test, RomBankMasks7Bits) {
  // Writing 0xFF should mask to 0x7F
  cart_->set(0x2000, 0xff);
  EXPECT_EQ(cart_->get(0x4000), 0x7f);
}

// ===== RAM Banking =====

TEST_F(Mbc3Test, RamDisabledByDefault) {
  EXPECT_EQ(cart_->get(0xa000), 0xff);
}

TEST_F(Mbc3Test, RamEnableAndAccess) {
  cart_->set(0x0000, 0x0a); // enable
  cart_->set(0x4000, 0x00); // select RAM bank 0
  cart_->set(0xa000, 0x42);
  EXPECT_EQ(cart_->get(0xa000), 0x42);
}

TEST_F(Mbc3Test, FourRamBanks) {
  cart_->set(0x0000, 0x0a);

  for (uint8_t bank = 0; bank < 4; bank++) {
    cart_->set(0x4000, bank);
    cart_->set(0xa000, bank + 0x20);
  }

  for (uint8_t bank = 0; bank < 4; bank++) {
    cart_->set(0x4000, bank);
    EXPECT_EQ(cart_->get(0xa000), bank + 0x20);
  }
}

// ===== RTC Tests =====

TEST_F(Mbc3Test, RtcLatchMechanism) {
  cart_->set(0x0000, 0x0a); // enable

  // Set base seconds to exactly 1h 2m 3s = 3723 seconds
  cart_->setRtcBaseSeconds(3723);
  // Set start time to now so elapsed = 0
  cart_->setRtcStartTime(std::chrono::system_clock::now());

  // Latch: write 0x00 then 0x01 to 0x6000-0x7FFF
  cart_->set(0x6000, 0x00);
  cart_->set(0x6000, 0x01);

  // Read latched seconds (register 0x08)
  cart_->set(0x4000, 0x08);
  EXPECT_EQ(cart_->get(0xa000), 3);

  // Read latched minutes (register 0x09)
  cart_->set(0x4000, 0x09);
  EXPECT_EQ(cart_->get(0xa000), 2);

  // Read latched hours (register 0x0A)
  cart_->set(0x4000, 0x0a);
  EXPECT_EQ(cart_->get(0xa000), 1);
}

TEST_F(Mbc3Test, RtcDaysCounter) {
  cart_->set(0x0000, 0x0a);

  // 300 days in seconds
  int64_t seconds_300_days = 300LL * 86400;
  cart_->setRtcBaseSeconds(seconds_300_days);
  cart_->setRtcStartTime(std::chrono::system_clock::now());

  cart_->set(0x6000, 0x00);
  cart_->set(0x6000, 0x01);

  // Days low (300 & 0xFF = 44)
  cart_->set(0x4000, 0x0b);
  EXPECT_EQ(cart_->get(0xa000), 300 & 0xff);

  // Days high (300 >> 8 = 1, bit 0 only)
  cart_->set(0x4000, 0x0c);
  EXPECT_EQ(cart_->get(0xa000) & 0x01, 1);
}

TEST_F(Mbc3Test, RtcCarryFlag) {
  cart_->set(0x0000, 0x0a);

  // 512 days -> should trigger carry
  int64_t seconds_512_days = 512LL * 86400;
  cart_->setRtcBaseSeconds(seconds_512_days);
  cart_->setRtcStartTime(std::chrono::system_clock::now());

  cart_->set(0x6000, 0x00);
  cart_->set(0x6000, 0x01);

  cart_->set(0x4000, 0x0c);
  uint8_t days_high = cart_->get(0xa000);
  EXPECT_TRUE(days_high & 0x80); // carry flag set
}

TEST_F(Mbc3Test, RtcLatchRequiresTwoStep) {
  cart_->set(0x0000, 0x0a);
  cart_->setRtcBaseSeconds(60); // 1 minute
  cart_->setRtcStartTime(std::chrono::system_clock::now());

  // Just writing 0x01 without 0x00 first should not latch
  cart_->set(0x6000, 0x01);

  // Latched registers should still be 0
  cart_->set(0x4000, 0x08);
  EXPECT_EQ(cart_->get(0xa000), 0);
}

TEST_F(Mbc3Test, RtcWriteRegisters) {
  cart_->set(0x0000, 0x0a);

  // Write to RTC seconds register
  cart_->set(0x4000, 0x08); // select RTC seconds
  cart_->set(0xa000, 30);   // set 30 seconds

  cart_->set(0x4000, 0x09); // select RTC minutes
  cart_->set(0xa000, 45);   // set 45 minutes

  // Latch and verify
  cart_->set(0x6000, 0x00);
  cart_->set(0x6000, 0x01);

  cart_->set(0x4000, 0x08);
  EXPECT_EQ(cart_->get(0xa000), 30);

  cart_->set(0x4000, 0x09);
  EXPECT_EQ(cart_->get(0xa000), 45);
}

TEST_F(Mbc3Test, RamAndRtcSelectSwitch) {
  cart_->set(0x0000, 0x0a);

  // Write to RAM bank 0
  cart_->set(0x4000, 0x00);
  cart_->set(0xa000, 0xaa);

  // Switch to RTC register
  cart_->set(0x4000, 0x08);
  // Latch first so we get clean RTC values
  cart_->setRtcBaseSeconds(0);
  cart_->setRtcStartTime(std::chrono::system_clock::now());
  cart_->set(0x6000, 0x00);
  cart_->set(0x6000, 0x01);
  EXPECT_EQ(cart_->get(0xa000), 0); // RTC seconds = 0

  // Switch back to RAM bank 0
  cart_->set(0x4000, 0x00);
  EXPECT_EQ(cart_->get(0xa000), 0xaa);
}

} // namespace cartridge
} // namespace mmu
