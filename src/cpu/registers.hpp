#pragma once

#include <cstdint>

namespace cpu {

// LR35902 CPU Register Set
//
// 8-bit registers: A, F, B, C, D, E, H, L
// 16-bit register pairs: AF, BC, DE, HL
// Special: SP (Stack Pointer), PC (Program Counter)
//
// F register flags (bits 7-4, bits 3-0 always zero):
//   Bit 7 - Z (Zero flag)
//   Bit 6 - N (Subtract flag)
//   Bit 5 - H (Half-carry flag)
//   Bit 4 - C (Carry flag)

class Registers {
public:
  uint8_t a = 0;
  uint8_t f = 0;
  uint8_t b = 0;
  uint8_t c = 0;
  uint8_t d = 0;
  uint8_t e = 0;
  uint8_t h = 0;
  uint8_t l = 0;
  uint16_t sp = 0;
  uint16_t pc = 0;

  // 16-bit register pair accessors
  uint16_t af() const { return (static_cast<uint16_t>(a) << 8) | f; }
  uint16_t bc() const { return (static_cast<uint16_t>(b) << 8) | c; }
  uint16_t de() const { return (static_cast<uint16_t>(d) << 8) | e; }
  uint16_t hl() const { return (static_cast<uint16_t>(h) << 8) | l; }

  void setAF(uint16_t val) {
    a = static_cast<uint8_t>(val >> 8);
    f = static_cast<uint8_t>(val & 0xF0); // Lower 4 bits of F are always 0
  }
  void setBC(uint16_t val) {
    b = static_cast<uint8_t>(val >> 8);
    c = static_cast<uint8_t>(val & 0xFF);
  }
  void setDE(uint16_t val) {
    d = static_cast<uint8_t>(val >> 8);
    e = static_cast<uint8_t>(val & 0xFF);
  }
  void setHL(uint16_t val) {
    h = static_cast<uint8_t>(val >> 8);
    l = static_cast<uint8_t>(val & 0xFF);
  }

  // Flag accessors
  bool flagZ() const { return (f >> 7) & 1; }
  bool flagN() const { return (f >> 6) & 1; }
  bool flagH() const { return (f >> 5) & 1; }
  bool flagC() const { return (f >> 4) & 1; }

  void setFlagZ(bool v) { f = (f & ~0x80) | (v ? 0x80 : 0); }
  void setFlagN(bool v) { f = (f & ~0x40) | (v ? 0x40 : 0); }
  void setFlagH(bool v) { f = (f & ~0x20) | (v ? 0x20 : 0); }
  void setFlagC(bool v) { f = (f & ~0x10) | (v ? 0x10 : 0); }

  // Set all flags at once
  void setFlags(bool z, bool n, bool h, bool c) {
    f = (z ? 0x80 : 0) | (n ? 0x40 : 0) | (h ? 0x20 : 0) | (c ? 0x10 : 0);
  }

  // Initialize to post-boot ROM values (DMG)
  void initDMG() {
    a = 0x01;
    f = 0xB0; // Z=1, N=0, H=1, C=1
    b = 0x00;
    c = 0x13;
    d = 0x00;
    e = 0xD8;
    h = 0x01;
    l = 0x4D;
    sp = 0xFFFE;
    pc = 0x0100; // Entry point after boot ROM
  }
};

} // namespace cpu
