#include "apu/channel1.hpp"
#include "state/serializable.hpp"

namespace apu {

constexpr uint8_t Channel1::DUTY_TABLE[4][8];

void Channel1::step(uint8_t cycles) {
  freq_timer_ -= cycles;
  while (freq_timer_ <= 0) {
    freq_timer_ += (2048 - frequency_) * 4;
    duty_index_ = (duty_index_ + 1) & 7;
  }
}

void Channel1::clockLength() {
  if (length_enable_ && length_counter_ > 0) {
    length_counter_--;
    if (length_counter_ == 0) {
      enabled_ = false;
    }
  }
}

void Channel1::clockEnvelope() {
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

uint16_t Channel1::calculateSweepFrequency() {
  uint16_t new_freq = sweep_shadow_ >> sweep_shift_;

  if (sweep_negate_) {
    new_freq = sweep_shadow_ - new_freq;
  } else {
    new_freq = sweep_shadow_ + new_freq;
  }

  // Overflow check: disable channel if freq > 2047
  if (new_freq > 2047) {
    enabled_ = false;
  }

  return new_freq;
}

void Channel1::clockSweep() {
  sweep_timer_--;
  if (sweep_timer_ == 0) {
    sweep_timer_ = sweep_period_ > 0 ? sweep_period_ : 8;

    if (sweep_enabled_ && sweep_period_ > 0) {
      uint16_t new_freq = calculateSweepFrequency();

      if (new_freq <= 2047 && sweep_shift_ > 0) {
        frequency_ = new_freq;
        sweep_shadow_ = new_freq;

        // Overflow check again with new frequency
        calculateSweepFrequency();
      }
    }
  }
}

void Channel1::trigger(uint8_t nr10, uint8_t nr11, uint8_t nr12,
                        uint16_t frequency, bool length_enable) {
  frequency_ = frequency;
  length_enable_ = length_enable;

  if (length_counter_ == 0) {
    length_counter_ = 64;
  }

  freq_timer_ = (2048 - frequency_) * 4;

  // Envelope
  envelope_init_ = (nr12 >> 4) & 0x0F;
  envelope_add_ = (nr12 & 0x08) != 0;
  envelope_period_ = nr12 & 0x07;
  volume_ = envelope_init_;
  envelope_timer_ = envelope_period_;

  // Sweep
  sweep_period_ = (nr10 >> 4) & 0x07;
  sweep_negate_ = (nr10 & 0x08) != 0;
  sweep_shift_ = nr10 & 0x07;
  sweep_shadow_ = frequency_;
  sweep_timer_ = sweep_period_ > 0 ? sweep_period_ : 8;
  sweep_enabled_ = (sweep_period_ > 0 || sweep_shift_ > 0);

  // If shift is non-zero, do overflow check immediately
  if (sweep_shift_ > 0) {
    calculateSweepFrequency();
  }

  // DAC check
  dac_on_ = (nr12 & 0xF8) != 0;
  enabled_ = dac_on_;

  duty_pattern_ = (nr11 >> 6) & 0x03;
}

void Channel1::writeNR10(uint8_t value) {
  sweep_period_ = (value >> 4) & 0x07;
  sweep_negate_ = (value & 0x08) != 0;
  sweep_shift_ = value & 0x07;
}

void Channel1::writeNR11(uint8_t value) {
  duty_pattern_ = (value >> 6) & 0x03;
  length_counter_ = 64 - (value & 0x3F);
}

void Channel1::writeNR12(uint8_t value) {
  dac_on_ = (value & 0xF8) != 0;
  if (!dac_on_) {
    enabled_ = false;
  }
  envelope_init_ = (value >> 4) & 0x0F;
  envelope_add_ = (value & 0x08) != 0;
  envelope_period_ = value & 0x07;
}

void Channel1::writeNR14(uint8_t value, uint8_t nr13) {
  length_enable_ = (value & 0x40) != 0;

  if (value & 0x80) {
    uint16_t freq = nr13 | (static_cast<uint16_t>(value & 0x07) << 8);
    uint8_t nr12_val =
        (envelope_init_ << 4) | (envelope_add_ ? 0x08 : 0) | envelope_period_;
    uint8_t nr10_val =
        (sweep_period_ << 4) | (sweep_negate_ ? 0x08 : 0) | sweep_shift_;
    trigger(nr10_val, 0, nr12_val, freq, length_enable_);
  }
}

float Channel1::getOutput() const {
  if (!enabled_ || !dac_on_)
    return 0.0f;

  uint8_t sample = DUTY_TABLE[duty_pattern_][duty_index_] * volume_;
  return (static_cast<float>(sample) / 7.5f) - 1.0f;
}

void Channel1::serialize(std::vector<uint8_t> &buf) const {
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
  state::write_u16(buf, sweep_shadow_);
  state::write_u8(buf, sweep_period_);
  state::write_bool(buf, sweep_negate_);
  state::write_u8(buf, sweep_shift_);
  state::write_u8(buf, sweep_timer_);
  state::write_bool(buf, sweep_enabled_);
}

void Channel1::deserialize(const uint8_t *data, size_t &pos) {
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
  sweep_shadow_ = state::read_u16(data, pos);
  sweep_period_ = state::read_u8(data, pos);
  sweep_negate_ = state::read_bool(data, pos);
  sweep_shift_ = state::read_u8(data, pos);
  sweep_timer_ = state::read_u8(data, pos);
  sweep_enabled_ = state::read_bool(data, pos);
}

} // namespace apu
