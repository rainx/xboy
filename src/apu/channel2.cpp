#include "apu/channel2.hpp"
#include "state/serializable.hpp"

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

void Channel2::serialize(std::vector<uint8_t> &buf) const {
  state::write_bool(buf, enabled_);
  state::write_bool(buf, dac_on_);
  state::write_u16(buf, frequency_);
  state::write_u32(buf, static_cast<uint32_t>(freq_timer_));
  state::write_u8(buf, duty_index_);
  state::write_u8(buf, duty_pattern_);
  state::write_u16(buf, length_counter_);
  state::write_bool(buf, length_enable_);
  state::write_u8(buf, volume_);
  state::write_u8(buf, envelope_init_);
  state::write_bool(buf, envelope_add_);
  state::write_u8(buf, envelope_period_);
  state::write_u8(buf, envelope_timer_);
}

void Channel2::deserialize(const uint8_t *data, size_t &pos) {
  enabled_ = state::read_bool(data, pos);
  dac_on_ = state::read_bool(data, pos);
  frequency_ = state::read_u16(data, pos);
  freq_timer_ = static_cast<int>(state::read_u32(data, pos));
  duty_index_ = state::read_u8(data, pos);
  duty_pattern_ = state::read_u8(data, pos);
  length_counter_ = state::read_u16(data, pos);
  length_enable_ = state::read_bool(data, pos);
  volume_ = state::read_u8(data, pos);
  envelope_init_ = state::read_u8(data, pos);
  envelope_add_ = state::read_bool(data, pos);
  envelope_period_ = state::read_u8(data, pos);
  envelope_timer_ = state::read_u8(data, pos);
}

} // namespace apu
