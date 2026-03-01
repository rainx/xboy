#pragma once

#include "cpu/cpu.hpp"
#include "debugger/disassembler.hpp"
#include "mmu/mmunit.hpp"

#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace debugger {

struct Breakpoint {
  int id;
  uint16_t address;
};

struct Watchpoint {
  int id;
  uint16_t address;
  uint8_t last_value;
};

class Debugger {
public:
  Debugger(cpu::Cpu &cpu, std::shared_ptr<mmu::Mmunit> mmu);

  // Check if debugger should pause before executing instruction at current PC.
  // Returns true if emulation should pause.
  bool shouldPause();

  // Enter interactive command loop. Returns false if user wants to quit.
  bool commandLoop();

  // Enable/disable debugger
  void setEnabled(bool enabled) { enabled_ = enabled; }
  bool isEnabled() const { return enabled_; }

  // Start paused (for --debug flag)
  void setPaused(bool paused) { paused_ = paused; }
  bool isPaused() const { return paused_; }

private:
  cpu::Cpu &cpu_;
  std::shared_ptr<mmu::Mmunit> mmu_;
  Disassembler disasm_;

  bool enabled_ = false;
  bool paused_ = false;
  bool step_mode_ = false;
  bool step_over_ = false;
  uint16_t step_over_return_addr_ = 0;

  std::vector<Breakpoint> breakpoints_;
  std::vector<Watchpoint> watchpoints_;
  int next_bp_id_ = 1;
  int next_wp_id_ = 1;

  // Command handlers
  void cmdStep();
  void cmdNext();
  void cmdContinue();
  void cmdBreak(const std::string &arg);
  void cmdDelete(const std::string &arg);
  void cmdList();
  void cmdRegs();
  void cmdMem(const std::string &args);
  void cmdDis(const std::string &args);
  void cmdWatch(const std::string &arg);
  void cmdHelp();

  void printCurrentInstruction();
  void checkWatchpoints();

  // Parse a hex string to uint16_t
  static bool parseHex(const std::string &s, uint16_t &out);
};

} // namespace debugger
