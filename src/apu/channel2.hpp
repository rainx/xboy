#pragma once

#include <cstdint>
#include <vector>

namespace apu {

// Channel 2: Square wave (no frequency sweep)
//
// Registers (accent addresses for Channel 2):
//   NR21 (0xFF16): Duty & length load  [DDLL LLLL]
//   NR22 (0xFF17): Volume envelope     [VVVV APPP]
//   NR23 (0xFF18): Frequency low       [FFFF FFFF]
//   NR24 (0xFF19): Trigger & freq high [TL-- -FFF]
//
// Duty patterns (8 steps each):
//   0: 00000001  (12.5%)
//   1: 00000011  (25%)
//   2: 00001111  (50%)
//   3: 11111100  (75%)

class Channel2 {
public:
  void step(uint8_t cycles);

  // Called by frame sequencer at 256 Hz
  void clockLength();
  // Called by frame sequencer at 64 Hz
  void clockEnvelope();

  // Trigger the channel (NRx4 bit 7)
  void trigger(uint8_t nr21, uint8_t nr22, uint16_t frequency, bool length_enable);

  // Write registers (called by APU when game writes NR2x)
  void writeNR21(uint8_t value);
  void writeNR22(uint8_t value);
  void writeNR24(uint8_t value, uint8_t nr23);

  // Get current output sample (0-15 DAC range, or 0 if off)
  float getOutput() const;

  bool isEnabled() const { return enabled_; }
  bool isDacOn() const { return dac_on_; }

  void serialize(std::vector<uint8_t> &buf) const;
  void deserialize(const uint8_t *data, size_t &pos);

private:
  static constexpr uint8_t DUTY_TABLE[4][8] = {
      {0, 0, 0, 0, 0, 0, 0, 1}, // 12.5%
      {1, 0, 0, 0, 0, 0, 0, 1}, // 25%
      {1, 0, 0, 0, 0, 1, 1, 1}, // 50%
      {0, 1, 1, 1, 1, 1, 1, 0}, // 75%
  };

  bool enabled_ = false;
  bool dac_on_ = false;

  // Frequency timer
  uint16_t frequency_ = 0;    // 11-bit frequency value from NR23/NR24
  int freq_timer_ = 0;        // Counts down; reloads with (2048 - frequency) * 4
  uint8_t duty_index_ = 0;    // Position in duty pattern (0-7)
  uint8_t duty_pattern_ = 0;  // Which duty pattern (0-3)

  // Length counter
  uint16_t length_counter_ = 0; // Counts down from 64
  bool length_enable_ = false;

  // Volume envelope
  uint8_t volume_ = 0;          // Current volume (0-15)
  uint8_t envelope_init_ = 0;   // Initial volume from NR22
  bool envelope_add_ = false;   // true = increase, false = decrease
  uint8_t envelope_period_ = 0; // Envelope sweep pace (0 = disabled)
  uint8_t envelope_timer_ = 0;  // Countdown timer
};

} // namespace apu
