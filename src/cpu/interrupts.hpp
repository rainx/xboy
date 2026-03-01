#pragma once

#include <cstdint>

namespace cpu {

// Game Boy Interrupt System
//
// IF register (0xFF0F): Interrupt Request Flags
// IE register (0xFFFF): Interrupt Enable
//
// Bit 0: V-Blank   (INT 0x40) — highest priority
// Bit 1: LCD STAT  (INT 0x48)
// Bit 2: Timer     (INT 0x50)
// Bit 3: Serial    (INT 0x58)
// Bit 4: Joypad    (INT 0x60) — lowest priority

enum InterruptFlag : uint8_t {
  VBlank = 0x01,  // Bit 0
  LcdStat = 0x02, // Bit 1
  Timer = 0x04,   // Bit 2
  Serial = 0x08,  // Bit 3
  Joypad = 0x10,  // Bit 4
};

} // namespace cpu
