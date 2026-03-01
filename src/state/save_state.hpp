#pragma once

#include "apu/apu.hpp"
#include "cpu/cpu.hpp"
#include "input/joypad.hpp"
#include "mmu/mmunit.hpp"
#include "ppu/ppu.hpp"
#include "timer/timer.hpp"

#include <memory>
#include <string>

namespace state {

// File format:
//   [4B magic "XBOY"] [2B version] [1B cartridge_type]
//   [CPU state] [MMU state] [Cartridge state] [PPU state]
//   [Timer state] [APU state] [Input state]

class SaveStateManager {
public:
  SaveStateManager(cpu::Cpu &cpu, std::shared_ptr<mmu::Mmunit> mmu,
                   ppu::Ppu &ppu, timer::Timer &tmr, apu::Apu &apu,
                   std::shared_ptr<input::Joypad> joypad,
                   const std::string &rom_path);

  // Save state to file. Returns true on success.
  bool save(int slot = 1);

  // Load state from file. Returns true on success.
  bool load(int slot = 1);

private:
  std::string getStatePath(int slot) const;

  cpu::Cpu &cpu_;
  std::shared_ptr<mmu::Mmunit> mmu_;
  ppu::Ppu &ppu_;
  timer::Timer &tmr_;
  apu::Apu &apu_;
  std::shared_ptr<input::Joypad> joypad_;
  std::string rom_path_;

  static constexpr uint8_t MAGIC[4] = {'X', 'B', 'O', 'Y'};
  static constexpr uint16_t VERSION = 1;
};

} // namespace state
