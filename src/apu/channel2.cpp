#include "apu/channel2.hpp"

namespace apu {

constexpr uint8_t Channel2::DUTY_TABLE[4][8];

void Channel2::step(uint8_t cycles) {
  freq_timer_ -= cycles;
  while (freq_timer_ <= 0) {
    freq_timer_ += (2048 - frequency_) * 4;
    duty_index_ = (duty_index_ + 1) & 7;
  }
}

void Channel2::clockLength() {
  if (length_enable_ && length_counter_ > 0) {
    length_counter_--;
    if (length_counter_ == 0) {
      enabled_ = false;
    }
  }
}

void Channel2::clockEnvelope() {
  if (envelope_period_ == 0)
    return;

  envelope_timer_--;
  if (envelope_timer_ == 0) {
    envelope_timer_ = envelope_period_;

    if (envelope_add_ && volume_ < 15) {
      volume_++;
    } else if (!envelope_add_ && volume_ > 0) {
      volume_--;
    }
  }
}

void Channel2::trigger(uint8_t nr21, uint8_t nr22, uint16_t frequency,
                        bool length_enable) {
  frequency_ = frequency;
  length_enable_ = length_enable;

  // Reload length counter if zero
  if (length_counter_ == 0) {
    length_counter_ = 64;
  }

  // Reload frequency timer
  freq_timer_ = (2048 - frequency_) * 4;

  // Reload envelope
  envelope_init_ = (nr22 >> 4) & 0x0F;
  envelope_add_ = (nr22 & 0x08) != 0;
  envelope_period_ = nr22 & 0x07;
  volume_ = envelope_init_;
  envelope_timer_ = envelope_period_;

  // DAC check: top 5 bits of NR22 must be non-zero
  dac_on_ = (nr22 & 0xF8) != 0;
  enabled_ = dac_on_;

  // Duty pattern
  duty_pattern_ = (nr21 >> 6) & 0x03;
}

void Channel2::writeNR21(uint8_t value) {
  duty_pattern_ = (value >> 6) & 0x03;
  length_counter_ = 64 - (value & 0x3F);
}

void Channel2::writeNR22(uint8_t value) {
  dac_on_ = (value & 0xF8) != 0;
  if (!dac_on_) {
    enabled_ = false;
  }
  envelope_init_ = (value >> 4) & 0x0F;
  envelope_add_ = (value & 0x08) != 0;
  envelope_period_ = value & 0x07;
}

void Channel2::writeNR24(uint8_t value, uint8_t nr23) {
  length_enable_ = (value & 0x40) != 0;

  if (value & 0x80) {
    // Trigger
    uint16_t freq = nr23 | (static_cast<uint16_t>(value & 0x07) << 8);
    // Read NR22 state already stored
    uint8_t nr22_val =
        (envelope_init_ << 4) | (envelope_add_ ? 0x08 : 0) | envelope_period_;
    trigger(0, nr22_val, freq, length_enable_);
    // duty_pattern_ is already set from writeNR21
  }
}

float Channel2::getOutput() const {
  if (!enabled_ || !dac_on_)
    return 0.0f;

  // DAC converts 0-15 to -1.0..+1.0
  uint8_t sample = DUTY_TABLE[duty_pattern_][duty_index_] * volume_;
  return (static_cast<float>(sample) / 7.5f) - 1.0f;
}

} // namespace apu
