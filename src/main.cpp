#include "cpu/cpu.hpp"
#include "input/joypad.hpp"
#include "mmu/mmunit.hpp"
#include "platform/sdl_display.hpp"
#include "platform/sdl_input.hpp"
#include "ppu/ppu.hpp"
#include "timer/timer.hpp"

#include <iostream>
#include <memory>
#include <string>

// Cycles per frame: 4194304 Hz / 59.7275 fps ≈ 70224
constexpr uint32_t CYCLES_PER_FRAME = 70224;

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: xboy <rom_path>" << std::endl;
    return 1;
  }

  std::string rom_path = argv[1];
  std::cout << "Loading ROM: " << rom_path << std::endl;

  // Initialize subsystems
  auto mmu = mmu::Mmunit::powerUp(rom_path);
  cpu::Cpu cpu_core(mmu);
  timer::Timer tmr(mmu);
  ppu::Ppu gpu(mmu);
  auto joypad = std::make_shared<input::Joypad>(mmu);

  platform::SdlDisplay display(3); // 3x scale → 480x432 window
  platform::SdlInput sdl_input(joypad);

  bool running = true;

  while (running) {
    uint32_t frame_cycles = 0;

    while (frame_cycles < CYCLES_PER_FRAME) {
      uint8_t cycles = cpu_core.step();
      tmr.step(cycles);
      gpu.step(cycles);
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
