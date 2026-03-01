#include "apu/apu.hpp"
#include "state/serializable.hpp"

namespace apu {

Apu::Apu(std::shared_ptr<mmu::Mmunit> mmu)
    : mmu_(mmu), ch3_(mmu) {}

void Apu::setSampleCallback(SampleCallback cb) {
  sample_callback_ = std::move(cb);
}

void Apu::step(uint8_t cycles) {
  // Check NR52 power status
  uint8_t nr52 = mmu_->get(reg::NR52);
  powered_ = (nr52 & 0x80) != 0;

  if (!powered_) {
    // When powered off, output silence
    sample_counter_ += static_cast<uint32_t>(cycles) * SAMPLE_RATE;
    while (sample_counter_ >= CPU_CLOCK) {
      sample_counter_ -= CPU_CLOCK;
      if (sample_callback_) {
        sample_callback_(0.0f, 0.0f);
      }
    }
    return;
  }

  // Check for register writes and triggers
  checkRegisterWrites();
  checkTriggers();

  // Step all channels
  ch1_.step(cycles);
  ch2_.step(cycles);
  ch3_.step(cycles);
  ch4_.step(cycles);

  // Step frame sequencer
  frame_seq_counter_ += cycles;
  while (frame_seq_counter_ >= FRAME_SEQ_PERIOD) {
    frame_seq_counter_ -= FRAME_SEQ_PERIOD;
    stepFrameSequencer();
  }

  // Downsample and output
  sample_counter_ += static_cast<uint32_t>(cycles) * SAMPLE_RATE;
  while (sample_counter_ >= CPU_CLOCK) {
    sample_counter_ -= CPU_CLOCK;
    mixAndOutput();
  }
}

void Apu::stepFrameSequencer() {
  switch (frame_seq_step_) {
  case 0:
    ch1_.clockLength();
    ch2_.clockLength();
    ch3_.clockLength();
    ch4_.clockLength();
    break;
  case 2:
    ch1_.clockLength();
    ch2_.clockLength();
    ch3_.clockLength();
    ch4_.clockLength();
    ch1_.clockSweep();
    break;
  case 4:
    ch1_.clockLength();
    ch2_.clockLength();
    ch3_.clockLength();
    ch4_.clockLength();
    break;
  case 6:
    ch1_.clockLength();
    ch2_.clockLength();
    ch3_.clockLength();
    ch4_.clockLength();
    ch1_.clockSweep();
    break;
  case 7:
    ch1_.clockEnvelope();
    ch2_.clockEnvelope();
    ch4_.clockEnvelope();
    break;
  }
  frame_seq_step_ = (frame_seq_step_ + 1) & 7;
}

void Apu::checkRegisterWrites() {
  // Channel 1 registers
  ch1_.writeNR10(mmu_->get(reg::NR10));
  ch1_.writeNR11(mmu_->get(reg::NR11));
  ch1_.writeNR12(mmu_->get(reg::NR12));

  // Channel 2 registers
  ch2_.writeNR21(mmu_->get(reg::NR21));
  ch2_.writeNR22(mmu_->get(reg::NR22));

  // Channel 3 registers
  ch3_.writeNR30(mmu_->get(reg::NR30));
  ch3_.writeNR31(mmu_->get(reg::NR31));
  ch3_.writeNR32(mmu_->get(reg::NR32));

  // Channel 4 registers
  ch4_.writeNR41(mmu_->get(reg::NR41));
  ch4_.writeNR42(mmu_->get(reg::NR42));
  ch4_.writeNR43(mmu_->get(reg::NR43));
}

void Apu::checkTriggers() {
  uint8_t nr14 = mmu_->get(reg::NR14);
  if ((nr14 & 0x80) && !(prev_nr14_ & 0x80)) {
    ch1_.writeNR14(nr14, mmu_->get(reg::NR13));
    // Clear trigger bit so we don't re-trigger
    mmu_->set(reg::NR14, nr14 & 0x7F);
  }
  prev_nr14_ = mmu_->get(reg::NR14);

  uint8_t nr24 = mmu_->get(reg::NR24);
  if ((nr24 & 0x80) && !(prev_nr24_ & 0x80)) {
    ch2_.writeNR24(nr24, mmu_->get(reg::NR23));
    mmu_->set(reg::NR24, nr24 & 0x7F);
  }
  prev_nr24_ = mmu_->get(reg::NR24);

  uint8_t nr34 = mmu_->get(reg::NR34);
  if ((nr34 & 0x80) && !(prev_nr34_ & 0x80)) {
    ch3_.writeNR34(nr34, mmu_->get(reg::NR33));
    mmu_->set(reg::NR34, nr34 & 0x7F);
  }
  prev_nr34_ = mmu_->get(reg::NR34);

  uint8_t nr44 = mmu_->get(reg::NR44);
  if ((nr44 & 0x80) && !(prev_nr44_ & 0x80)) {
    ch4_.writeNR44(nr44);
    mmu_->set(reg::NR44, nr44 & 0x7F);
  }
  prev_nr44_ = mmu_->get(reg::NR44);
}

void Apu::mixAndOutput() {
  if (!sample_callback_)
    return;

  // Get individual channel outputs (-1.0 to +1.0)
  float ch1_out = ch1_.getOutput();
  float ch2_out = ch2_.getOutput();
  float ch3_out = ch3_.getOutput();
  float ch4_out = ch4_.getOutput();

  // NR51: Channel panning
  // Bit 7: CH4 left,  Bit 6: CH3 left,  Bit 5: CH2 left,  Bit 4: CH1 left
  // Bit 3: CH4 right, Bit 2: CH3 right, Bit 1: CH2 right, Bit 0: CH1 right
  uint8_t nr51 = mmu_->get(reg::NR51);

  float left = 0.0f;
  float right = 0.0f;

  if (nr51 & 0x10) left += ch1_out;
  if (nr51 & 0x20) left += ch2_out;
  if (nr51 & 0x40) left += ch3_out;
  if (nr51 & 0x80) left += ch4_out;

  if (nr51 & 0x01) right += ch1_out;
  if (nr51 & 0x02) right += ch2_out;
  if (nr51 & 0x04) right += ch3_out;
  if (nr51 & 0x08) right += ch4_out;

  // NR50: Master volume (0-7 for each side)
  // Bits 6-4: left volume, Bits 2-0: right volume
  uint8_t nr50 = mmu_->get(reg::NR50);
  uint8_t left_vol = (nr50 >> 4) & 0x07;
  uint8_t right_vol = nr50 & 0x07;

  left *= (static_cast<float>(left_vol + 1) / 8.0f);
  right *= (static_cast<float>(right_vol + 1) / 8.0f);

  // Scale down: 4 channels mixed, keep within -1..+1 range
  left *= 0.25f;
  right *= 0.25f;

  // Update NR52 channel status bits (read-only bits 0-3)
  uint8_t nr52 = mmu_->get(reg::NR52) & 0x80; // Preserve power bit
  if (ch1_.isEnabled()) nr52 |= 0x01;
  if (ch2_.isEnabled()) nr52 |= 0x02;
  if (ch3_.isEnabled()) nr52 |= 0x04;
  if (ch4_.isEnabled()) nr52 |= 0x08;
  mmu_->set(reg::NR52, nr52);

  sample_callback_(left, right);
}

void Apu::serialize(std::vector<uint8_t> &buf) const {
  state::write_u16(buf, frame_seq_counter_);
  state::write_u8(buf, frame_seq_step_);
  state::write_u32(buf, sample_counter_);
  state::write_u8(buf, prev_nr14_);
  state::write_u8(buf, prev_nr24_);
  state::write_u8(buf, prev_nr34_);
  state::write_u8(buf, prev_nr44_);
  state::write_bool(buf, powered_);
  ch1_.serialize(buf);
  ch2_.serialize(buf);
  ch3_.serialize(buf);
  ch4_.serialize(buf);
}

void Apu::deserialize(const uint8_t *data, size_t &pos) {
  frame_seq_counter_ = state::read_u16(data, pos);
  frame_seq_step_ = state::read_u8(data, pos);
  sample_counter_ = state::read_u32(data, pos);
  prev_nr14_ = state::read_u8(data, pos);
  prev_nr24_ = state::read_u8(data, pos);
  prev_nr34_ = state::read_u8(data, pos);
  prev_nr44_ = state::read_u8(data, pos);
  powered_ = state::read_bool(data, pos);
  ch1_.deserialize(data, pos);
  ch2_.deserialize(data, pos);
  ch3_.deserialize(data, pos);
  ch4_.deserialize(data, pos);
}

} // namespace apu
