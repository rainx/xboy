#include "apu/apu.hpp"
#include "cpu/cpu.hpp"
#include "debugger/debugger.hpp"
#include "input/joypad.hpp"
#include "mmu/mmunit.hpp"
#include "platform/sdl_audio.hpp"
#include "platform/sdl_display.hpp"
#include "platform/sdl_input.hpp"
#include "ppu/ppu.hpp"
#include "state/save_state.hpp"
#include "test_runner/headless_runner.hpp"
#include "timer/timer.hpp"

#include <iostream>
#include <memory>
#include <string>

// Cycles per frame: 4194304 Hz / 59.7275 fps ≈ 70224
constexpr uint32_t CYCLES_PER_FRAME = 70224;

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: xboy [--test|--debug] <rom_path>" << std::endl;
    return 1;
  }

  // Parse CLI arguments
  bool test_mode = false;
  bool debug_mode = false;
  std::string rom_path;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--test") {
      test_mode = true;
    } else if (arg == "--debug") {
      debug_mode = true;
    } else {
      rom_path = arg;
    }
  }

  if (rom_path.empty()) {
    std::cerr << "Error: no ROM path specified" << std::endl;
    return 1;
  }

  // Headless test mode: run Blargg test ROM and exit
  if (test_mode) {
    test_runner::HeadlessRunner runner(rom_path);
    auto result = runner.run();

    if (!result.serial_output.empty()) {
      std::cout << result.serial_output << std::endl;
    }

    switch (result.result) {
    case test_runner::TestResult::Passed:
      std::cout << "TEST PASSED (" << result.cycles_run << " cycles)"
                << std::endl;
      return 0;
    case test_runner::TestResult::Failed:
      std::cout << "TEST FAILED (" << result.cycles_run << " cycles)"
                << std::endl;
      return 1;
    case test_runner::TestResult::Timeout:
      std::cout << "TEST TIMEOUT (" << result.cycles_run << " cycles)"
                << std::endl;
      return 1;
    case test_runner::TestResult::InfiniteLoop:
      std::cout << "INFINITE LOOP detected (" << result.cycles_run
                << " cycles)" << std::endl;
      return 1;
    }
    return 1;
  }
  std::cout << "Loading ROM: " << rom_path << std::endl;

  // Initialize subsystems
  auto mmu = mmu::Mmunit::powerUp(rom_path);
  cpu::Cpu cpu_core(mmu);
  timer::Timer tmr(mmu);
  ppu::Ppu gpu(mmu);
  apu::Apu audio(mmu);
  auto joypad = std::make_shared<input::Joypad>(mmu);

  platform::SdlDisplay display(3); // 3x scale → 480x432 window
  platform::SdlAudio sdl_audio;
  platform::SdlInput sdl_input(joypad);

  // Save state manager
  state::SaveStateManager save_mgr(cpu_core, mmu, gpu, tmr, audio, joypad,
                                   rom_path);

  // Debugger
  debugger::Debugger dbg(cpu_core, mmu);
  if (debug_mode) {
    dbg.setEnabled(true);
    dbg.setPaused(true);
    std::cout << "Debugger active. Type 'h' for help." << std::endl;
  }

  // Wire APU sample output to SDL audio queue
  audio.setSampleCallback(
      [&sdl_audio](float left, float right) {
        sdl_audio.pushSample(left, right);
      });

  bool running = true;

  while (running) {
    uint32_t frame_cycles = 0;

    while (frame_cycles < CYCLES_PER_FRAME) {
      // Debugger: check for breakpoints before stepping
      if (dbg.shouldPause()) {
        if (!dbg.commandLoop()) {
          running = false;
          break;
        }
      }

      uint8_t cycles = cpu_core.step();
      tmr.step(cycles);
      gpu.step(cycles);
      audio.step(cycles);
      frame_cycles += cycles;

      uint8_t int_cycles = cpu_core.handleInterrupts();
      frame_cycles += int_cycles;

      // Update joypad register (in case game wrote to FF00 select bits)
      joypad->update();
    }

    // Render frame
    if (gpu.frameReady()) {
      display.render(gpu.getFrameBuffer());
      gpu.clearFrameReady();
    }

    // Poll input and handle quit
    running = sdl_input.poll();

    // Handle save/load state
    if (sdl_input.saveRequested()) {
      save_mgr.save();
    }
    if (sdl_input.loadRequested()) {
      save_mgr.load();
    }

    // Toggle debugger with F12
    if (sdl_input.debugToggleRequested()) {
      bool was_enabled = dbg.isEnabled();
      dbg.setEnabled(!was_enabled);
      dbg.setPaused(!was_enabled);
      std::cout << (was_enabled ? "Debugger disabled." : "Debugger enabled.")
                << std::endl;
    }

    // Audio-based throttling: wait if audio queue is too far ahead
    // This naturally syncs A/V — audio queue depth controls frame pacing
    while (sdl_audio.getQueuedBytes() > platform::SdlAudio::SAMPLE_RATE * 4 * 2 / 15) {
      SDL_Delay(1);
    }

    // Frame timing
    display.syncFrame();
  }

  // Print serial output (for Blargg test ROMs)
  std::string serial = mmu->getSerialOutput();
  if (!serial.empty()) {
    std::cout << "Serial output:\n" << serial << std::endl;
  }

  return 0;
}
