#pragma once

#include "apu/channel1.hpp"
#include "apu/channel2.hpp"
#include "apu/channel3.hpp"
#include "apu/channel4.hpp"
#include "mmu/mmunit.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace apu {

// Game Boy APU register addresses
namespace reg {
// Channel 1 — Square wave with sweep
constexpr uint16_t NR10 = 0xFF10;
constexpr uint16_t NR11 = 0xFF11;
constexpr uint16_t NR12 = 0xFF12;
constexpr uint16_t NR13 = 0xFF13;
constexpr uint16_t NR14 = 0xFF14;

// Channel 2 — Square wave
constexpr uint16_t NR21 = 0xFF16;
constexpr uint16_t NR22 = 0xFF17;
constexpr uint16_t NR23 = 0xFF18;
constexpr uint16_t NR24 = 0xFF19;

// Channel 3 — Wave
constexpr uint16_t NR30 = 0xFF1A;
constexpr uint16_t NR31 = 0xFF1B;
constexpr uint16_t NR32 = 0xFF1C;
constexpr uint16_t NR33 = 0xFF1D;
constexpr uint16_t NR34 = 0xFF1E;

// Channel 4 — Noise
constexpr uint16_t NR41 = 0xFF20;
constexpr uint16_t NR42 = 0xFF21;
constexpr uint16_t NR43 = 0xFF22;
constexpr uint16_t NR44 = 0xFF23;

// Master control
constexpr uint16_t NR50 = 0xFF24; // Master volume / VIN panning
constexpr uint16_t NR51 = 0xFF25; // Channel panning (L/R selection)
constexpr uint16_t NR52 = 0xFF26; // Audio master enable + channel status
} // namespace reg

// Sample callback: receives (left, right) stereo floats
using SampleCallback = std::function<void(float, float)>;

class Apu {
public:
  explicit Apu(std::shared_ptr<mmu::Mmunit> mmu);

  // Advance APU by the given number of T-cycles
  void step(uint8_t cycles);

  // Set the callback that receives downsampled audio samples
  void setSampleCallback(SampleCallback cb);

  // Save state serialization
  void serialize(std::vector<uint8_t> &buf) const;
  void deserialize(const uint8_t *data, size_t &pos);

private:
  std::shared_ptr<mmu::Mmunit> mmu_;

  Channel1 ch1_;
  Channel2 ch2_;
  Channel3 ch3_;
  Channel4 ch4_;

  SampleCallback sample_callback_;

  // Frame sequencer: 8192 T-cycles per step (512 Hz)
  static constexpr uint16_t FRAME_SEQ_PERIOD = 8192;
  uint16_t frame_seq_counter_ = 0;
  uint8_t frame_seq_step_ = 0; // 0-7

  // Downsampling: Bresenham accumulator
  // CPU clock = 4194304 Hz, target sample rate = 44100 Hz
  static constexpr uint32_t CPU_CLOCK = 4194304;
  static constexpr uint32_t SAMPLE_RATE = 44100;
  uint32_t sample_counter_ = 0;

  // Previous register values for trigger detection
  uint8_t prev_nr14_ = 0;
  uint8_t prev_nr24_ = 0;
  uint8_t prev_nr34_ = 0;
  uint8_t prev_nr44_ = 0;

  bool powered_ = true;

  void stepFrameSequencer();
  void checkTriggers();
  void checkRegisterWrites();
  void mixAndOutput();
};

} // namespace apu
