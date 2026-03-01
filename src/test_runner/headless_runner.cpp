#include "test_runner/headless_runner.hpp"
#include "apu/apu.hpp"
#include "cpu/cpu.hpp"
#include "input/joypad.hpp"
#include "mmu/mmunit.hpp"
#include "ppu/ppu.hpp"
#include "timer/timer.hpp"

namespace test_runner {

HeadlessRunner::HeadlessRunner(const std::string &rom_path)
    : rom_path_(rom_path) {}

TestRunResult HeadlessRunner::run(uint64_t max_cycles) {
  auto mmu = mmu::Mmunit::powerUp(rom_path_);
  cpu::Cpu cpu_core(mmu);
  timer::Timer tmr(mmu);
  ppu::Ppu gpu(mmu);
  apu::Apu audio(mmu);
  auto joypad = std::make_shared<input::Joypad>(mmu);

  // No SDL — APU samples are discarded (no callback set)

  uint64_t total_cycles = 0;
  uint16_t prev_pc = cpu_core.regs().pc;
  uint32_t stall_count = 0;
  constexpr uint32_t STALL_THRESHOLD = 10'000;

  while (total_cycles < max_cycles) {
    uint8_t cycles = cpu_core.step();
    tmr.step(cycles);
    gpu.step(cycles);
    audio.step(cycles);
    total_cycles += cycles;

    uint8_t int_cycles = cpu_core.handleInterrupts();
    total_cycles += int_cycles;

    joypad->update();

    // Check for infinite loop (PC unchanged for many cycles)
    uint16_t current_pc = cpu_core.regs().pc;
    if (current_pc == prev_pc) {
      stall_count++;
      if (stall_count > STALL_THRESHOLD) {
        std::string serial = mmu->getSerialOutput();
        TestResult result = TestResult::InfiniteLoop;
        if (serial.find("Passed") != std::string::npos) {
          result = TestResult::Passed;
        } else if (serial.find("Failed") != std::string::npos) {
          result = TestResult::Failed;
        }
        return {result, serial, total_cycles};
      }
    } else {
      stall_count = 0;
      prev_pc = current_pc;
    }

    // Check serial output for early termination
    const std::string &serial = mmu->getSerialOutput();
    if (serial.find("Passed") != std::string::npos) {
      return {TestResult::Passed, serial, total_cycles};
    }
    if (serial.find("Failed") != std::string::npos) {
      return {TestResult::Failed, serial, total_cycles};
    }
  }

  std::string serial = mmu->getSerialOutput();
  TestResult result = TestResult::Timeout;
  if (serial.find("Passed") != std::string::npos) {
    result = TestResult::Passed;
  } else if (serial.find("Failed") != std::string::npos) {
    result = TestResult::Failed;
  }
  return {result, serial, total_cycles};
}

} // namespace test_runner
