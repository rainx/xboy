#include "apu/channel3.hpp"

namespace apu {

Channel3::Channel3(std::shared_ptr<mmu::Mmunit> mmu) : mmu_(mmu) {}

void Channel3::step(uint8_t cycles) {
  freq_timer_ -= cycles;
  while (freq_timer_ <= 0) {
    freq_timer_ += (2048 - frequency_) * 2;
    sample_index_ = (sample_index_ + 1) & 31;
  }
}

void Channel3::clockLength() {
  if (length_enable_ && length_counter_ > 0) {
    length_counter_--;
    if (length_counter_ == 0) {
      enabled_ = false;
    }
  }
}

void Channel3::trigger(uint16_t frequency, bool length_enable) {
  frequency_ = frequency;
  length_enable_ = length_enable;

  if (length_counter_ == 0) {
    length_counter_ = 256;
  }

  freq_timer_ = (2048 - frequency_) * 2;
  sample_index_ = 0;

  enabled_ = dac_on_;
}

void Channel3::writeNR30(uint8_t value) {
  dac_on_ = (value & 0x80) != 0;
  if (!dac_on_) {
    enabled_ = false;
  }
}

void Channel3::writeNR31(uint8_t value) {
  length_counter_ = 256 - value;
}

void Channel3::writeNR32(uint8_t value) {
  volume_code_ = (value >> 5) & 0x03;
}

void Channel3::writeNR34(uint8_t value, uint8_t nr33) {
  length_enable_ = (value & 0x40) != 0;

  if (value & 0x80) {
    uint16_t freq = nr33 | (static_cast<uint16_t>(value & 0x07) << 8);
    trigger(freq, length_enable_);
  }
}

float Channel3::getOutput() const {
  if (!enabled_ || !dac_on_)
    return 0.0f;

  // Read 4-bit sample from wave RAM
  // Each byte holds two samples: high nibble first
  uint8_t byte_index = sample_index_ / 2;
  uint8_t wave_byte = mmu_->get(0xFF30 + byte_index);

  uint8_t sample;
  if ((sample_index_ & 1) == 0) {
    sample = (wave_byte >> 4) & 0x0F; // High nibble
  } else {
    sample = wave_byte & 0x0F; // Low nibble
  }

  // Apply volume shift
  // 0 = mute (shift 4 = 0), 1 = 100% (shift 0), 2 = 50% (shift 1), 3 = 25%
  // (shift 2)
  static constexpr uint8_t VOLUME_SHIFT[4] = {4, 0, 1, 2};
  sample >>= VOLUME_SHIFT[volume_code_];

  // DAC converts 0-15 to -1.0..+1.0
  return (static_cast<float>(sample) / 7.5f) - 1.0f;
}

} // namespace apu
