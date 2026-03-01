#include "apu/channel4.hpp"

namespace apu {

constexpr uint16_t Channel4::DIVISOR_TABLE[8];

void Channel4::step(uint8_t cycles) {
  freq_timer_ -= cycles;
  while (freq_timer_ <= 0) {
    freq_timer_ += DIVISOR_TABLE[divisor_code_] << clock_shift_;

    // Clock the LFSR
    uint8_t xor_result = (lfsr_ & 0x01) ^ ((lfsr_ >> 1) & 0x01);
    lfsr_ >>= 1;
    lfsr_ |= (xor_result << 14); // Set bit 14

    if (width_mode_) {
      // 7-bit mode: also set bit 6
      lfsr_ = (lfsr_ & ~(1 << 6)) | (xor_result << 6);
    }
  }
}

void Channel4::clockLength() {
  if (length_enable_ && length_counter_ > 0) {
    length_counter_--;
    if (length_counter_ == 0) {
      enabled_ = false;
    }
  }
}

void Channel4::clockEnvelope() {
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

void Channel4::trigger(uint8_t nr42, uint8_t nr43, bool length_enable) {
  length_enable_ = length_enable;

  if (length_counter_ == 0) {
    length_counter_ = 64;
  }

  // Clock config
  clock_shift_ = (nr43 >> 4) & 0x0F;
  width_mode_ = (nr43 & 0x08) != 0;
  divisor_code_ = nr43 & 0x07;

  freq_timer_ = DIVISOR_TABLE[divisor_code_] << clock_shift_;

  // Reset LFSR
  lfsr_ = 0x7FFF;

  // Envelope
  envelope_init_ = (nr42 >> 4) & 0x0F;
  envelope_add_ = (nr42 & 0x08) != 0;
  envelope_period_ = nr42 & 0x07;
  volume_ = envelope_init_;
  envelope_timer_ = envelope_period_;

  // DAC check
  dac_on_ = (nr42 & 0xF8) != 0;
  enabled_ = dac_on_;
}

void Channel4::writeNR41(uint8_t value) {
  length_counter_ = 64 - (value & 0x3F);
}

void Channel4::writeNR42(uint8_t value) {
  dac_on_ = (value & 0xF8) != 0;
  if (!dac_on_) {
    enabled_ = false;
  }
  envelope_init_ = (value >> 4) & 0x0F;
  envelope_add_ = (value & 0x08) != 0;
  envelope_period_ = value & 0x07;
}

void Channel4::writeNR43(uint8_t value) {
  clock_shift_ = (value >> 4) & 0x0F;
  width_mode_ = (value & 0x08) != 0;
  divisor_code_ = value & 0x07;
}

void Channel4::writeNR44(uint8_t value) {
  length_enable_ = (value & 0x40) != 0;

  if (value & 0x80) {
    uint8_t nr42_val =
        (envelope_init_ << 4) | (envelope_add_ ? 0x08 : 0) | envelope_period_;
    uint8_t nr43_val =
        (clock_shift_ << 4) | (width_mode_ ? 0x08 : 0) | divisor_code_;
    trigger(nr42_val, nr43_val, length_enable_);
  }
}

float Channel4::getOutput() const {
  if (!enabled_ || !dac_on_)
    return 0.0f;

  // LFSR output: bit 0 inverted (0 = high, 1 = low)
  uint8_t sample = (~lfsr_ & 0x01) * volume_;
  return (static_cast<float>(sample) / 7.5f) - 1.0f;
}

} // namespace apu
