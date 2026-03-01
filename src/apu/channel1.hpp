#pragma once

#include <cstdint>

namespace apu {

// Channel 1: Square wave with frequency sweep
//
// Registers:
//   NR10 (0xFF10): Sweep          [0PPP NSSS]
//   NR11 (0xFF11): Duty & length  [DDLL LLLL]
//   NR12 (0xFF12): Volume envelope [VVVV APPP]
//   NR13 (0xFF13): Frequency low  [FFFF FFFF]
//   NR14 (0xFF14): Trigger & high [TL-- -FFF]

class Channel1 {
public:
  void step(uint8_t cycles);

  // Frame sequencer clocks
  void clockLength();
  void clockEnvelope();
  void clockSweep();

  void trigger(uint8_t nr10, uint8_t nr11, uint8_t nr12, uint16_t frequency,
               bool length_enable);

  void writeNR10(uint8_t value);
  void writeNR11(uint8_t value);
  void writeNR12(uint8_t value);
  void writeNR14(uint8_t value, uint8_t nr13);

  float getOutput() const;

  bool isEnabled() const { return enabled_; }
  bool isDacOn() const { return dac_on_; }

private:
  static constexpr uint8_t DUTY_TABLE[4][8] = {
      {0, 0, 0, 0, 0, 0, 0, 1}, // 12.5%
      {1, 0, 0, 0, 0, 0, 0, 1}, // 25%
      {1, 0, 0, 0, 0, 1, 1, 1}, // 50%
      {0, 1, 1, 1, 1, 1, 1, 0}, // 75%
  };

  uint16_t calculateSweepFrequency();

  bool enabled_ = false;
  bool dac_on_ = false;

  // Frequency timer
  uint16_t frequency_ = 0;
  int freq_timer_ = 0;
  uint8_t duty_index_ = 0;
  uint8_t duty_pattern_ = 0;

  // Length counter
  uint16_t length_counter_ = 0;
  bool length_enable_ = false;

  // Volume envelope
  uint8_t volume_ = 0;
  uint8_t envelope_init_ = 0;
  bool envelope_add_ = false;
  uint8_t envelope_period_ = 0;
  uint8_t envelope_timer_ = 0;

  // Sweep
  uint16_t sweep_shadow_ = 0;    // Shadow frequency register
  uint8_t sweep_period_ = 0;     // Sweep pace (0 = disabled)
  bool sweep_negate_ = false;    // true = subtract
  uint8_t sweep_shift_ = 0;     // Shift amount (0-7)
  uint8_t sweep_timer_ = 0;     // Countdown timer
  bool sweep_enabled_ = false;  // Internal enable flag
};

} // namespace apu
