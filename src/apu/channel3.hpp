#pragma once

#include "mmu/mmunit.hpp"
#include <cstdint>
#include <memory>
#include <vector>

namespace apu {

// Channel 3: Programmable wave channel
//
// Registers:
//   NR30 (0xFF1A): DAC enable     [E--- ----]
//   NR31 (0xFF1B): Length load    [LLLL LLLL]
//   NR32 (0xFF1C): Volume code    [-VV- ----]
//   NR33 (0xFF1D): Frequency low  [FFFF FFFF]
//   NR34 (0xFF1E): Trigger & high [TL-- -FFF]
//
// Wave RAM: 0xFF30-0xFF3F (16 bytes = 32 x 4-bit samples)

class Channel3 {
public:
  explicit Channel3(std::shared_ptr<mmu::Mmunit> mmu);

  void step(uint8_t cycles);
  void clockLength();

  void trigger(uint16_t frequency, bool length_enable);

  void writeNR30(uint8_t value);
  void writeNR31(uint8_t value);
  void writeNR32(uint8_t value);
  void writeNR34(uint8_t value, uint8_t nr33);

  float getOutput() const;

  bool isEnabled() const { return enabled_; }
  bool isDacOn() const { return dac_on_; }

  void serialize(std::vector<uint8_t> &buf) const;
  void deserialize(const uint8_t *data, size_t &pos);

private:
  std::shared_ptr<mmu::Mmunit> mmu_;

  bool enabled_ = false;
  bool dac_on_ = false;

  // Frequency timer
  uint16_t frequency_ = 0;
  int freq_timer_ = 0;
  uint8_t sample_index_ = 0; // Position in wave table (0-31)

  // Length counter (Channel 3 has 256-step length)
  uint16_t length_counter_ = 0;
  bool length_enable_ = false;

  // Volume shift: 0=mute, 1=100%, 2=50%, 3=25%
  uint8_t volume_code_ = 0;
};

} // namespace apu
