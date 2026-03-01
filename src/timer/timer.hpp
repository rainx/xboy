#pragma once

#include "mmu/mmunit.hpp"
#include <cstdint>
#include <memory>
#include <vector>

namespace timer {

// Game Boy Timer
//
// DIV  (0xFF04): Divider register — increments at 16384 Hz (every 256 T-cycles)
// TIMA (0xFF05): Timer counter — increments at frequency set by TAC
// TMA  (0xFF06): Timer modulo — value loaded into TIMA on overflow
// TAC  (0xFF07): Timer control
//   Bit 2: Timer enable
//   Bits 1-0: Clock select
//     00 = 4096 Hz   (every 1024 T-cycles)
//     01 = 262144 Hz (every 16 T-cycles)
//     10 = 65536 Hz  (every 64 T-cycles)
//     11 = 16384 Hz  (every 256 T-cycles)

class Timer {
public:
  Timer(std::shared_ptr<mmu::Mmunit> mmu);

  // Advance timer by the given number of T-cycles
  void step(uint8_t cycles);

  // Save state serialization
  void serialize(std::vector<uint8_t> &buf) const;
  void deserialize(const uint8_t *data, size_t &pos);

private:
  std::shared_ptr<mmu::Mmunit> mmu_;
  uint16_t div_counter_ = 0;  // Internal counter for DIV (counts T-cycles)
  uint16_t tima_counter_ = 0; // Internal counter for TIMA

  // TAC clock select → T-cycles per TIMA increment
  static uint16_t getTimaThreshold(uint8_t tac);
};

} // namespace timer
