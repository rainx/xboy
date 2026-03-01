#include "cpu/cpu.hpp"
#include "cpu/registers.hpp"
#include "mmu/mmunit.hpp"
#include "gtest/gtest.h"

#include <array>
#include <memory>

// Minimal cartridge for testing: loads a small program into ROM
class TestCartridge : public mmu::cartridge::Cartridge {
public:
  TestCartridge()
      : Cartridge(mmu::cartridge::RomOnly, "test"),
        rom_(std::make_shared<std::array<uint8_t, 0x8000>>()) {
    rom_->fill(0);
  }

  void loadProgram(const std::vector<uint8_t> &program, uint16_t offset = 0x0100) {
    for (size_t i = 0; i < program.size(); i++) {
      (*rom_)[offset + i] = program[i];
    }
  }

  uint8_t get(const uint16_t &address) const override {
    return (*rom_)[address];
  }

  void set(const uint16_t &address, const uint8_t value) override {}
  void save() override {}
  void serialize(std::vector<uint8_t> &) const override {}
  void deserialize(const uint8_t *, size_t &) override {}

private:
  std::shared_ptr<std::array<uint8_t, 0x8000>> rom_;
};

class CpuTest : public ::testing::Test {
protected:
  std::shared_ptr<TestCartridge> cart;
  std::shared_ptr<mmu::Mmunit> mmu;
  std::unique_ptr<cpu::Cpu> cpu_core;

  void SetUp() override {
    cart = std::make_shared<TestCartridge>();
    mmu = std::make_shared<mmu::Mmunit>(cart);
    cpu_core = std::make_unique<cpu::Cpu>(mmu);
  }

  void loadAndRun(const std::vector<uint8_t> &program, int steps = 1) {
    cart->loadProgram(program);
    for (int i = 0; i < steps; i++) {
      cpu_core->step();
    }
  }
};

// ============================================================
// Register initialization
// ============================================================

TEST_F(CpuTest, InitialRegisters) {
  EXPECT_EQ(cpu_core->regs().a, 0x01);
  EXPECT_EQ(cpu_core->regs().f, 0xB0);
  EXPECT_EQ(cpu_core->regs().b, 0x00);
  EXPECT_EQ(cpu_core->regs().c, 0x13);
  EXPECT_EQ(cpu_core->regs().sp, 0xFFFE);
  EXPECT_EQ(cpu_core->regs().pc, 0x0100);
}

// ============================================================
// NOP
// ============================================================

TEST_F(CpuTest, NOP) {
  loadAndRun({0x00}); // NOP
  EXPECT_EQ(cpu_core->regs().pc, 0x0101);
}

// ============================================================
// LD immediate
// ============================================================

TEST_F(CpuTest, LD_B_d8) {
  loadAndRun({0x06, 0x42}); // LD B, 0x42
  EXPECT_EQ(cpu_core->regs().b, 0x42);
}

TEST_F(CpuTest, LD_BC_d16) {
  loadAndRun({0x01, 0x34, 0x12}); // LD BC, 0x1234
  EXPECT_EQ(cpu_core->regs().bc(), 0x1234);
}

TEST_F(CpuTest, LD_SP_d16) {
  loadAndRun({0x31, 0x00, 0xC0}); // LD SP, 0xC000
  EXPECT_EQ(cpu_core->regs().sp, 0xC000);
}

// ============================================================
// LD register-to-register
// ============================================================

TEST_F(CpuTest, LD_A_B) {
  loadAndRun({0x06, 0x55, 0x78}, 2); // LD B, 0x55; LD A, B
  EXPECT_EQ(cpu_core->regs().a, 0x55);
}

// ============================================================
// ADD / SUB / AND / OR / XOR / CP
// ============================================================

TEST_F(CpuTest, ADD_A_B) {
  // A starts at 0x01, load B=0x0F, then ADD A,B
  loadAndRun({0x06, 0x0F, 0x80}, 2);
  EXPECT_EQ(cpu_core->regs().a, 0x10);
  EXPECT_FALSE(cpu_core->regs().flagZ());
  EXPECT_FALSE(cpu_core->regs().flagN());
  EXPECT_TRUE(cpu_core->regs().flagH()); // half-carry from 0x01+0x0F
  EXPECT_FALSE(cpu_core->regs().flagC());
}

TEST_F(CpuTest, SUB_B) {
  // A=0x01, B=0x01 → A=0, Z=1, N=1
  loadAndRun({0x06, 0x01, 0x90}, 2);
  EXPECT_EQ(cpu_core->regs().a, 0x00);
  EXPECT_TRUE(cpu_core->regs().flagZ());
  EXPECT_TRUE(cpu_core->regs().flagN());
}

TEST_F(CpuTest, AND_B) {
  // A=0x01(0b00000001), B=0xF0 → A=0x00
  loadAndRun({0x06, 0xF0, 0xA0}, 2);
  EXPECT_EQ(cpu_core->regs().a, 0x00);
  EXPECT_TRUE(cpu_core->regs().flagZ());
  EXPECT_TRUE(cpu_core->regs().flagH()); // AND always sets H
}

TEST_F(CpuTest, XOR_A) {
  loadAndRun({0xAF}); // XOR A (A ^= A → 0)
  EXPECT_EQ(cpu_core->regs().a, 0x00);
  EXPECT_TRUE(cpu_core->regs().flagZ());
}

TEST_F(CpuTest, CP_B) {
  // A=0x01, B=0x01 → Z=1 (but A unchanged)
  loadAndRun({0x06, 0x01, 0xB8}, 2);
  EXPECT_EQ(cpu_core->regs().a, 0x01); // A not modified
  EXPECT_TRUE(cpu_core->regs().flagZ());
  EXPECT_TRUE(cpu_core->regs().flagN());
}

// ============================================================
// INC / DEC
// ============================================================

TEST_F(CpuTest, INC_B) {
  loadAndRun({0x06, 0xFF, 0x04}, 2); // B=0xFF, INC B → 0x00
  EXPECT_EQ(cpu_core->regs().b, 0x00);
  EXPECT_TRUE(cpu_core->regs().flagZ());
  EXPECT_TRUE(cpu_core->regs().flagH());
}

TEST_F(CpuTest, DEC_B) {
  loadAndRun({0x06, 0x01, 0x05}, 2); // B=0x01, DEC B → 0x00
  EXPECT_EQ(cpu_core->regs().b, 0x00);
  EXPECT_TRUE(cpu_core->regs().flagZ());
  EXPECT_TRUE(cpu_core->regs().flagN());
}

TEST_F(CpuTest, INC_BC) {
  loadAndRun({0x01, 0xFF, 0xFF, 0x03}, 2); // BC=0xFFFF, INC BC → 0x0000
  EXPECT_EQ(cpu_core->regs().bc(), 0x0000);
  // 16-bit INC does not affect flags
}

// ============================================================
// Jumps
// ============================================================

TEST_F(CpuTest, JP_a16) {
  loadAndRun({0xC3, 0x00, 0x02}); // JP 0x0200
  EXPECT_EQ(cpu_core->regs().pc, 0x0200);
}

TEST_F(CpuTest, JR_r8) {
  loadAndRun({0x18, 0x05}); // JR +5 → PC was at 0x102 after fetch, now 0x107
  EXPECT_EQ(cpu_core->regs().pc, 0x0107);
}

TEST_F(CpuTest, JR_NZ_taken) {
  // A=0x01, XOR A won't run. Just test JR NZ with Z=0 (initial F=0xB0 has Z=1)
  // First clear Z: LD A, 1 already has A=1, but Z is set from init.
  // Use OR A to clear Z (A=0x01, Z=0)
  loadAndRun({0xB7, 0x20, 0x03}, 2); // OR A; JR NZ, +3
  EXPECT_EQ(cpu_core->regs().pc, 0x0106); // 0x0103 + 3
}

TEST_F(CpuTest, JR_NZ_not_taken) {
  // A=0x01, XOR A → Z=1, then JR NZ should not jump
  loadAndRun({0xAF, 0x20, 0x05}, 2); // XOR A; JR NZ, +5
  EXPECT_EQ(cpu_core->regs().pc, 0x0103); // not taken, just past the JR
}

// ============================================================
// CALL / RET
// ============================================================

TEST_F(CpuTest, CALL_and_RET) {
  // Set SP, then CALL, then RET
  // Program at 0x0100: LD SP, 0xFFFE; CALL 0x0200
  // Program at 0x0200: RET
  cart->loadProgram({0x31, 0xFE, 0xFF, 0xCD, 0x00, 0x02}, 0x0100);
  cart->loadProgram({0xC9}, 0x0200); // RET

  cpu_core->step(); // LD SP
  cpu_core->step(); // CALL 0x0200
  EXPECT_EQ(cpu_core->regs().pc, 0x0200);
  EXPECT_EQ(cpu_core->regs().sp, 0xFFFC); // pushed 2 bytes

  cpu_core->step(); // RET
  EXPECT_EQ(cpu_core->regs().pc, 0x0106); // returned to after CALL
  EXPECT_EQ(cpu_core->regs().sp, 0xFFFE);
}

// ============================================================
// PUSH / POP
// ============================================================

TEST_F(CpuTest, PUSH_POP_BC) {
  cart->loadProgram({
      0x31, 0xFE, 0xFF, // LD SP, 0xFFFE
      0x01, 0x34, 0x12, // LD BC, 0x1234
      0xC5,             // PUSH BC
      0x01, 0x00, 0x00, // LD BC, 0x0000
      0xC1,             // POP BC
  });

  for (int i = 0; i < 5; i++)
    cpu_core->step();

  EXPECT_EQ(cpu_core->regs().bc(), 0x1234);
}

// ============================================================
// LD (HL+) / LD (HL-)
// ============================================================

TEST_F(CpuTest, LD_HLplus_A) {
  // HL=0xC000, A=0x42, LD (HL+), A
  cart->loadProgram({
      0x21, 0x00, 0xC0, // LD HL, 0xC000
      0x3E, 0x42,       // LD A, 0x42
      0x22,             // LD (HL+), A
  });

  for (int i = 0; i < 3; i++)
    cpu_core->step();

  EXPECT_EQ(mmu->get(0xC000), 0x42);
  EXPECT_EQ(cpu_core->regs().hl(), 0xC001);
}

// ============================================================
// LDH (high page I/O)
// ============================================================

TEST_F(CpuTest, LDH_a8_A) {
  cart->loadProgram({
      0x3E, 0x80,  // LD A, 0x80
      0xE0, 0x50,  // LDH (0x50), A → write A to 0xFF50
  });

  cpu_core->step();
  cpu_core->step();

  EXPECT_EQ(mmu->get(0xFF50), 0x80);
}

// ============================================================
// CB prefix: SWAP, BIT, SET, RES
// ============================================================

TEST_F(CpuTest, CB_SWAP_A) {
  cart->loadProgram({
      0x3E, 0xF0,     // LD A, 0xF0
      0xCB, 0x37,     // SWAP A → 0x0F
  });

  cpu_core->step();
  cpu_core->step();

  EXPECT_EQ(cpu_core->regs().a, 0x0F);
  EXPECT_FALSE(cpu_core->regs().flagZ());
}

TEST_F(CpuTest, CB_BIT_7_A) {
  cart->loadProgram({
      0x3E, 0x80,     // LD A, 0x80
      0xCB, 0x7F,     // BIT 7, A → bit 7 is set, Z=0
  });

  cpu_core->step();
  cpu_core->step();

  EXPECT_FALSE(cpu_core->regs().flagZ());
  EXPECT_TRUE(cpu_core->regs().flagH());
  EXPECT_FALSE(cpu_core->regs().flagN());
}

TEST_F(CpuTest, CB_SET_RES) {
  cart->loadProgram({
      0x3E, 0x00,     // LD A, 0x00
      0xCB, 0xC7,     // SET 0, A → A=0x01
      0xCB, 0x87,     // RES 0, A → A=0x00
  });

  cpu_core->step(); // LD A, 0
  cpu_core->step(); // SET 0, A
  EXPECT_EQ(cpu_core->regs().a, 0x01);

  cpu_core->step(); // RES 0, A
  EXPECT_EQ(cpu_core->regs().a, 0x00);
}

// ============================================================
// Rotate instructions
// ============================================================

TEST_F(CpuTest, RLCA) {
  cart->loadProgram({
      0x3E, 0x85,  // LD A, 0x85 (10000101)
      0x07,        // RLCA → 0x0B (00001011), C=1
  });

  cpu_core->step();
  cpu_core->step();

  EXPECT_EQ(cpu_core->regs().a, 0x0B);
  EXPECT_TRUE(cpu_core->regs().flagC());
  EXPECT_FALSE(cpu_core->regs().flagZ()); // RLCA always clears Z
}

TEST_F(CpuTest, CB_RL_A) {
  cart->loadProgram({
      0x3E, 0x80,     // LD A, 0x80 (10000000)
      0xCB, 0x17,     // RL A → rotate left through carry
  });

  // Initial carry from F=0xB0 is C=1
  cpu_core->step();
  cpu_core->step();

  // RL: bit7→C, old_C→bit0. A=0x80, C=1 → result = 0x01, new C=1
  EXPECT_EQ(cpu_core->regs().a, 0x01);
  EXPECT_TRUE(cpu_core->regs().flagC());
}

// ============================================================
// DAA (Decimal Adjust Accumulator)
// ============================================================

TEST_F(CpuTest, DAA_after_ADD) {
  // 0x15 + 0x27 = 0x3C → after DAA: 0x42 (BCD: 15+27=42)
  cart->loadProgram({
      0x3E, 0x15,  // LD A, 0x15
      0x06, 0x27,  // LD B, 0x27
      0x80,        // ADD A, B → 0x3C
      0x27,        // DAA → 0x42
  });

  for (int i = 0; i < 4; i++)
    cpu_core->step();

  EXPECT_EQ(cpu_core->regs().a, 0x42);
}

// ============================================================
// HALT
// ============================================================

TEST_F(CpuTest, HALT) {
  loadAndRun({0x76}); // HALT
  EXPECT_TRUE(cpu_core->isHalted());
  EXPECT_EQ(cpu_core->regs().pc, 0x0101);
}

// ============================================================
// Memory read/write through WRAM
// ============================================================

TEST_F(CpuTest, LD_HL_A_roundtrip) {
  cart->loadProgram({
      0x21, 0x00, 0xC0,  // LD HL, 0xC000
      0x3E, 0xAB,        // LD A, 0xAB
      0x77,              // LD (HL), A
      0x3E, 0x00,        // LD A, 0x00
      0x7E,              // LD A, (HL)
  });

  for (int i = 0; i < 5; i++)
    cpu_core->step();

  EXPECT_EQ(cpu_core->regs().a, 0xAB);
}

// ============================================================
// RST
// ============================================================

TEST_F(CpuTest, RST_38) {
  cart->loadProgram({
      0x31, 0xFE, 0xFF,  // LD SP, 0xFFFE
      0xFF,              // RST 38H
  });

  cpu_core->step(); // LD SP
  cpu_core->step(); // RST 38

  EXPECT_EQ(cpu_core->regs().pc, 0x0038);
}

// ============================================================
// ADD SP, r8
// ============================================================

TEST_F(CpuTest, ADD_SP_r8) {
  cart->loadProgram({
      0x31, 0x00, 0xC0,  // LD SP, 0xC000
      0xE8, 0xFE,        // ADD SP, -2
  });

  cpu_core->step();
  cpu_core->step();

  EXPECT_EQ(cpu_core->regs().sp, 0xBFFE);
  EXPECT_FALSE(cpu_core->regs().flagZ());
  EXPECT_FALSE(cpu_core->regs().flagN());
}

// ============================================================
// POP AF (lower nibble masking)
// ============================================================

TEST_F(CpuTest, POP_AF_masks_lower_nibble) {
  cart->loadProgram({
      0x31, 0xFE, 0xFF,  // LD SP, 0xFFFE
      0x01, 0xFF, 0x12,  // LD BC, 0x12FF
      0xC5,              // PUSH BC
      0xF1,              // POP AF → A=0x12, F=0xF0 (lower nibble masked)
  });

  for (int i = 0; i < 4; i++)
    cpu_core->step();

  EXPECT_EQ(cpu_core->regs().a, 0x12);
  EXPECT_EQ(cpu_core->regs().f, 0xF0); // Lower 4 bits always 0
}

// ============================================================
// Interrupt handling
// ============================================================

TEST_F(CpuTest, InterruptDispatch) {
  cart->loadProgram({
      0x31, 0xFE, 0xFF,  // LD SP, 0xFFFE
      0xFB,              // EI
      0x00,              // NOP (EI takes effect after this)
      0x00,              // NOP (interrupt fires here)
  });

  // At 0x0040 (VBlank handler): just RET
  cart->loadProgram({0xC9}, 0x0040);

  cpu_core->step(); // LD SP
  cpu_core->step(); // EI (sets ei_pending)
  cpu_core->step(); // NOP (IME now enabled)

  // Set VBlank interrupt request
  mmu->set(0xFFFF, 0x01); // IE: enable VBlank
  mmu->set(0xFF0F, 0x01); // IF: request VBlank

  cpu_core->handleInterrupts();
  EXPECT_EQ(cpu_core->regs().pc, 0x0040);
  EXPECT_FALSE(cpu_core->ime()); // IME disabled during handler
}
