#pragma once

#include <cstdint>

namespace apu {

// Channel 4: Noise (LFSR-based)
//
// Registers:
//   NR41 (0xFF20): Length load    [--LL LLLL]
//   NR42 (0xFF21): Volume envelope [VVVV APPP]
//   NR43 (0xFF22): Clock config   [SSSS WDDD]
//     S = clock shift, W = width (0=15-bit, 1=7-bit), D = divisor code
//   NR44 (0xFF23): Trigger        [TL-- ----]

class Channel4 {
public:
  void step(uint8_t cycles);
  void clockLength();
  void clockEnvelope();

  void trigger(uint8_t nr42, uint8_t nr43, bool length_enable);

  void writeNR41(uint8_t value);
  void writeNR42(uint8_t value);
  void writeNR43(uint8_t value);
  void writeNR44(uint8_t value);

  float getOutput() const;

  bool isEnabled() const { return enabled_; }
  bool isDacOn() const { return dac_on_; }

private:
  // Divisor code → base divisor value
  static constexpr uint16_t DIVISOR_TABLE[8] = {8, 16, 32, 48, 64, 80, 96, 112};

  bool enabled_ = false;
  bool dac_on_ = false;

  // LFSR
  uint16_t lfsr_ = 0x7FFF; // 15-bit shift register, all 1s
  bool width_mode_ = false; // false=15-bit, true=7-bit
  uint8_t clock_shift_ = 0;
  uint8_t divisor_code_ = 0;

  // Frequency timer
  int freq_timer_ = 0;

  // Length counter
  uint16_t length_counter_ = 0;
  bool length_enable_ = false;

  // Volume envelope
  uint8_t volume_ = 0;
  uint8_t envelope_init_ = 0;
  bool envelope_add_ = false;
  uint8_t envelope_period_ = 0;
  uint8_t envelope_timer_ = 0;
};

} // namespace apu
