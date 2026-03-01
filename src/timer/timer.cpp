#include "timer/timer.hpp"
#include "cpu/interrupts.hpp"

namespace timer {

Timer::Timer(std::shared_ptr<mmu::Mmunit> mmu) : mmu_(mmu) {}

void Timer::step(uint8_t cycles) {
  // DIV increments every 256 T-cycles
  div_counter_ += cycles;
  while (div_counter_ >= 256) {
    div_counter_ -= 256;
    uint8_t div = mmu_->get(0xFF04);
    mmu_->set(0xFF04, div + 1);
  }

  // TIMA only runs if timer is enabled (TAC bit 2)
  uint8_t tac = mmu_->get(0xFF07);
  if (!(tac & 0x04)) {
    return;
  }

  uint16_t threshold = getTimaThreshold(tac);
  tima_counter_ += cycles;

  while (tima_counter_ >= threshold) {
    tima_counter_ -= threshold;
    uint8_t tima = mmu_->get(0xFF05);

    if (tima == 0xFF) {
      // Overflow: reload from TMA and request Timer interrupt
      mmu_->set(0xFF05, mmu_->get(0xFF06));
      uint8_t if_reg = mmu_->get(0xFF0F);
      mmu_->set(0xFF0F, if_reg | cpu::InterruptFlag::Timer);
    } else {
      mmu_->set(0xFF05, tima + 1);
    }
  }
}

uint16_t Timer::getTimaThreshold(uint8_t tac) {
  switch (tac & 0x03) {
  case 0: return 1024; // 4096 Hz
  case 1: return 16;   // 262144 Hz
  case 2: return 64;   // 65536 Hz
  case 3: return 256;  // 16384 Hz
  default: return 1024;
  }
}

} // namespace timer
