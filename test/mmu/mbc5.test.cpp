
#include "gtest/gtest.h"

#include "mmu/cartridge/mbc5.hpp"

namespace mmu {
namespace cartridge {

// Type alias to disambiguate from the CartridgeType enum value
using Mbc5Cart = class Mbc5;

class Mbc5Test : public ::testing::Test {
protected:
  void SetUp() override {
    rom_ = std::make_shared<array<uint8_t, max_mbc5_rom_size>>();
    rom_->fill(0);

    // Write distinct values into each bank's first byte for identification
    for (int bank = 0; bank < 512; bank++) {
      (*rom_)[bank * 0x4000] = bank & 0xff;
      (*rom_)[bank * 0x4000 + 1] = (bank >> 8) & 0xff;
    }

    cart_ = std::make_shared<Mbc5Cart>(Mbc5WithRam, rom_, "/tmp/mbc5_test");
  }

  std::shared_ptr<array<uint8_t, max_mbc5_rom_size>> rom_;
  std::shared_ptr<Mbc5Cart> cart_;
};

TEST_F(Mbc5Test, ReadFromBank0) {
  // Bank 0 is always at 0x0000-0x3FFF
  EXPECT_EQ(cart_->get(0x0000), 0x00);
  EXPECT_EQ(cart_->get(0x0001), 0x00);
}

TEST_F(Mbc5Test, DefaultRomBankIs1) {
  // Default rom bank should be 1
  EXPECT_EQ(cart_->get(0x4000), 0x01);
  EXPECT_EQ(cart_->get(0x4001), 0x00);
}

TEST_F(Mbc5Test, RomBankSwitchingLow8Bits) {
  // Switch to bank 5 via low 8 bits
  cart_->set(0x2000, 0x05);
  EXPECT_EQ(cart_->get(0x4000), 0x05);
}

TEST_F(Mbc5Test, RomBank0IsValid) {
  // Unlike MBC1, bank 0 IS valid in the switchable region
  cart_->set(0x2000, 0x00);
  EXPECT_EQ(cart_->get(0x4000), 0x00);
  EXPECT_EQ(cart_->get(0x4001), 0x00);
}

TEST_F(Mbc5Test, NineBitRomBanking) {
  // Set bank 256 (0x100): low bits = 0x00, high bit = 1
  cart_->set(0x3000, 0x01); // bit 8
  cart_->set(0x2000, 0x00); // low 8 bits
  // Bank 256: first byte should be 0x00, second byte 0x01
  EXPECT_EQ(cart_->get(0x4000), 0x00);
  EXPECT_EQ(cart_->get(0x4001), 0x01);

  // Set bank 257 (0x101)
  cart_->set(0x2000, 0x01);
  EXPECT_EQ(cart_->get(0x4000), 0x01);
  EXPECT_EQ(cart_->get(0x4001), 0x01);
}

TEST_F(Mbc5Test, RamDisabledByDefault) {
  // RAM should return 0xFF when disabled
  EXPECT_EQ(cart_->get(0xa000), 0xff);
}

TEST_F(Mbc5Test, RamEnableDisable) {
  // Enable RAM
  cart_->set(0x0000, 0x0a);
  cart_->set(0xa000, 0x42);
  EXPECT_EQ(cart_->get(0xa000), 0x42);

  // Disable RAM
  cart_->set(0x0000, 0x00);
  EXPECT_EQ(cart_->get(0xa000), 0xff);
}

TEST_F(Mbc5Test, RamBankSwitching) {
  cart_->set(0x0000, 0x0a); // enable RAM

  // Write to bank 0
  cart_->set(0xa000, 0xaa);

  // Switch to bank 1
  cart_->set(0x4000, 0x01);
  cart_->set(0xa000, 0xbb);

  EXPECT_EQ(cart_->get(0xa000), 0xbb);

  // Switch back to bank 0
  cart_->set(0x4000, 0x00);
  EXPECT_EQ(cart_->get(0xa000), 0xaa);
}

TEST_F(Mbc5Test, SixteenRamBanks) {
  cart_->set(0x0000, 0x0a); // enable RAM

  // Write distinct values to all 16 banks
  for (uint8_t bank = 0; bank < 16; bank++) {
    cart_->set(0x4000, bank);
    cart_->set(0xa000, bank + 0x10);
  }

  // Read them all back
  for (uint8_t bank = 0; bank < 16; bank++) {
    cart_->set(0x4000, bank);
    EXPECT_EQ(cart_->get(0xa000), bank + 0x10);
  }
}

TEST_F(Mbc5Test, RumbleDetection) {
  auto rumble_cart =
      std::make_shared<Mbc5Cart>(Mbc5WithRumble, rom_, "/tmp/mbc5_rumble_test");

  rumble_cart->set(0x0000, 0x0a); // enable RAM

  // With rumble: bit 3 is motor, bits 0-2 are bank (max 8 banks)
  // Set bank 3 with motor on (0x08 | 0x03 = 0x0B)
  rumble_cart->set(0x4000, 0x0b);
  rumble_cart->set(0xa000, 0xcc);

  // Reading bank 3 should get our value (motor bit is ignored for banking)
  rumble_cart->set(0x4000, 0x03);
  EXPECT_EQ(rumble_cart->get(0xa000), 0xcc);
}

} // namespace cartridge
} // namespace mmu
