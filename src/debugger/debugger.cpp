#include "debugger/debugger.hpp"

#include <algorithm>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace debugger {

Debugger::Debugger(cpu::Cpu &cpu, std::shared_ptr<mmu::Mmunit> mmu)
    : cpu_(cpu), mmu_(mmu), disasm_(mmu) {}

bool Debugger::shouldPause() {
  if (!enabled_) return false;

  // Step-over mode: pause when we return to the step-over address
  if (step_over_) {
    if (cpu_.regs().pc == step_over_return_addr_) {
      step_over_ = false;
      paused_ = true;
      return true;
    }
    return false;
  }

  // Single-step mode
  if (step_mode_) {
    paused_ = true;
    step_mode_ = false;
    return true;
  }

  // Check breakpoints
  uint16_t pc = cpu_.regs().pc;
  for (const auto &bp : breakpoints_) {
    if (bp.address == pc) {
      std::cout << "Breakpoint " << bp.id << " hit at $" << std::hex
                << std::setw(4) << std::setfill('0') << pc << std::dec
                << std::endl;
      paused_ = true;
      return true;
    }
  }

  // Check watchpoints
  for (auto &wp : watchpoints_) {
    uint8_t current = mmu_->get(wp.address);
    if (current != wp.last_value) {
      std::cout << "Watchpoint " << wp.id << ": $" << std::hex
                << std::setw(4) << std::setfill('0') << wp.address
                << " changed: $" << std::setw(2) << std::setfill('0')
                << static_cast<int>(wp.last_value) << " -> $"
                << std::setw(2) << std::setfill('0')
                << static_cast<int>(current) << std::dec << std::endl;
      wp.last_value = current;
      paused_ = true;
      return true;
    }
  }

  return paused_;
}

bool Debugger::commandLoop() {
  printCurrentInstruction();

  while (true) {
    std::cout << "(xdb) " << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) {
      return false; // EOF
    }

    // Trim whitespace
    size_t start = line.find_first_not_of(" \t");
    if (start == std::string::npos) continue;
    line = line.substr(start);

    // Split command and args
    std::string cmd, args;
    size_t space = line.find(' ');
    if (space != std::string::npos) {
      cmd = line.substr(0, space);
      args = line.substr(space + 1);
    } else {
      cmd = line;
    }

    if (cmd == "s" || cmd == "step") {
      cmdStep();
      return true;
    } else if (cmd == "n" || cmd == "next") {
      cmdNext();
      return true;
    } else if (cmd == "c" || cmd == "continue") {
      cmdContinue();
      return true;
    } else if (cmd == "b" || cmd == "break") {
      cmdBreak(args);
    } else if (cmd == "d" || cmd == "delete") {
      cmdDelete(args);
    } else if (cmd == "l" || cmd == "list") {
      cmdList();
    } else if (cmd == "r" || cmd == "regs") {
      cmdRegs();
    } else if (cmd == "m" || cmd == "mem") {
      cmdMem(args);
    } else if (cmd == "dis") {
      cmdDis(args);
    } else if (cmd == "w" || cmd == "watch") {
      cmdWatch(args);
    } else if (cmd == "h" || cmd == "help") {
      cmdHelp();
    } else if (cmd == "q" || cmd == "quit") {
      enabled_ = false;
      paused_ = false;
      std::cout << "Debugger disabled." << std::endl;
      return true;
    } else {
      std::cout << "Unknown command: " << cmd << ". Type 'h' for help."
                << std::endl;
    }
  }
}

void Debugger::cmdStep() {
  step_mode_ = true;
  paused_ = false;
}

void Debugger::cmdNext() {
  // Check if current instruction is a CALL
  uint8_t opcode = mmu_->get(cpu_.regs().pc);
  bool is_call = (opcode == 0xCD ||             // CALL nn
                  opcode == 0xC4 || opcode == 0xCC ||  // CALL NZ/Z
                  opcode == 0xD4 || opcode == 0xDC);   // CALL NC/C

  if (is_call) {
    // Set return address (instruction after the 3-byte CALL)
    step_over_ = true;
    step_over_return_addr_ = cpu_.regs().pc + 3;
    paused_ = false;
  } else {
    // Not a CALL — behave like step
    cmdStep();
  }
}

void Debugger::cmdContinue() {
  paused_ = false;
  step_mode_ = false;
}

void Debugger::cmdBreak(const std::string &arg) {
  uint16_t addr;
  if (!parseHex(arg, addr)) {
    std::cout << "Usage: break <hex_address>" << std::endl;
    return;
  }
  int id = next_bp_id_++;
  breakpoints_.push_back({id, addr});
  std::cout << "Breakpoint " << id << " set at $" << std::hex << std::setw(4)
            << std::setfill('0') << addr << std::dec << std::endl;
}

void Debugger::cmdDelete(const std::string &arg) {
  int id;
  try {
    id = std::stoi(arg);
  } catch (...) {
    std::cout << "Usage: delete <breakpoint_id>" << std::endl;
    return;
  }

  // Try breakpoints first
  auto bp_it = std::find_if(breakpoints_.begin(), breakpoints_.end(),
                            [id](const Breakpoint &bp) { return bp.id == id; });
  if (bp_it != breakpoints_.end()) {
    std::cout << "Deleted breakpoint " << id << std::endl;
    breakpoints_.erase(bp_it);
    return;
  }

  // Try watchpoints
  auto wp_it = std::find_if(watchpoints_.begin(), watchpoints_.end(),
                            [id](const Watchpoint &wp) { return wp.id == id; });
  if (wp_it != watchpoints_.end()) {
    std::cout << "Deleted watchpoint " << id << std::endl;
    watchpoints_.erase(wp_it);
    return;
  }

  std::cout << "No breakpoint/watchpoint with id " << id << std::endl;
}

void Debugger::cmdList() {
  if (breakpoints_.empty() && watchpoints_.empty()) {
    std::cout << "No breakpoints or watchpoints set." << std::endl;
    return;
  }
  for (const auto &bp : breakpoints_) {
    std::cout << "  Breakpoint " << bp.id << ": $" << std::hex << std::setw(4)
              << std::setfill('0') << bp.address << std::dec << std::endl;
  }
  for (const auto &wp : watchpoints_) {
    std::cout << "  Watchpoint " << wp.id << ": $" << std::hex << std::setw(4)
              << std::setfill('0') << wp.address << " (last=$"
              << std::setw(2) << std::setfill('0')
              << static_cast<int>(wp.last_value) << ")" << std::dec
              << std::endl;
  }
}

void Debugger::cmdRegs() {
  const auto &r = cpu_.regs();
  std::printf("  AF=%04X  BC=%04X  DE=%04X  HL=%04X\n", r.af(), r.bc(),
              r.de(), r.hl());
  std::printf("  SP=%04X  PC=%04X\n", r.sp, r.pc);
  std::printf("  Flags: Z=%d N=%d H=%d C=%d  IME=%d\n", r.flagZ() ? 1 : 0,
              r.flagN() ? 1 : 0, r.flagH() ? 1 : 0, r.flagC() ? 1 : 0,
              cpu_.ime() ? 1 : 0);
  std::printf("  HALT=%d STOP=%d\n", cpu_.isHalted() ? 1 : 0,
              cpu_.isStopped() ? 1 : 0);
}

void Debugger::cmdMem(const std::string &args) {
  std::istringstream iss(args);
  std::string addr_str;
  int count = 16;
  iss >> addr_str;
  iss >> count;

  uint16_t addr;
  if (!parseHex(addr_str, addr)) {
    std::cout << "Usage: mem <hex_address> [count]" << std::endl;
    return;
  }

  for (int i = 0; i < count; i += 16) {
    std::printf("  %04X: ", (addr + i) & 0xFFFF);
    for (int j = 0; j < 16 && (i + j) < count; j++) {
      std::printf("%02X ", mmu_->get((addr + i + j) & 0xFFFF));
    }
    std::printf("\n");
  }
}

void Debugger::cmdDis(const std::string &args) {
  std::istringstream iss(args);
  std::string addr_str;
  int count = 10;

  if (iss >> addr_str) {
    iss >> count;
  }

  uint16_t addr;
  if (addr_str.empty()) {
    addr = cpu_.regs().pc;
  } else if (!parseHex(addr_str, addr)) {
    std::cout << "Usage: dis [hex_address] [count]" << std::endl;
    return;
  }

  auto lines = disasm_.disassembleN(addr, count);
  for (const auto &line : lines) {
    std::printf("  $%04X: %s\n", line.address, line.mnemonic.c_str());
  }
}

void Debugger::cmdWatch(const std::string &arg) {
  uint16_t addr;
  if (!parseHex(arg, addr)) {
    std::cout << "Usage: watch <hex_address>" << std::endl;
    return;
  }
  int id = next_wp_id_++;
  uint8_t current_val = mmu_->get(addr);
  watchpoints_.push_back({id, addr, current_val});
  std::cout << "Watchpoint " << id << " set at $" << std::hex << std::setw(4)
            << std::setfill('0') << addr << " (current=$" << std::setw(2)
            << std::setfill('0') << static_cast<int>(current_val) << ")"
            << std::dec << std::endl;
}

void Debugger::cmdHelp() {
  std::cout << "Commands:\n"
            << "  s/step          Step one instruction\n"
            << "  n/next          Step over CALL\n"
            << "  c/continue      Run until breakpoint\n"
            << "  b/break <addr>  Set breakpoint (hex address)\n"
            << "  d/delete <id>   Remove breakpoint/watchpoint\n"
            << "  l/list          List breakpoints/watchpoints\n"
            << "  r/regs          Show CPU registers\n"
            << "  m/mem <addr> [n] Hex dump n bytes\n"
            << "  dis [addr] [n]  Disassemble n instructions\n"
            << "  w/watch <addr>  Watch memory address for changes\n"
            << "  q/quit          Exit debugger\n"
            << "  h/help          This help\n";
}

void Debugger::printCurrentInstruction() {
  auto line = disasm_.disassemble(cpu_.regs().pc);
  std::printf("=> $%04X: %s\n", line.address, line.mnemonic.c_str());
}

bool Debugger::parseHex(const std::string &s, uint16_t &out) {
  if (s.empty()) return false;
  try {
    // Remove optional $ or 0x prefix
    std::string clean = s;
    if (clean[0] == '$') clean = clean.substr(1);
    if (clean.size() > 2 && clean[0] == '0' && (clean[1] == 'x' || clean[1] == 'X')) {
      clean = clean.substr(2);
    }
    unsigned long val = std::stoul(clean, nullptr, 16);
    if (val > 0xFFFF) return false;
    out = static_cast<uint16_t>(val);
    return true;
  } catch (...) {
    return false;
  }
}

} // namespace debugger
