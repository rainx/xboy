#pragma once

#include "mmu/mmunit.hpp"
#include <cstdint>
#include <memory>
#include <string>

namespace debugger {

struct DisasmLine {
  uint16_t address;
  std::string mnemonic;
  uint8_t bytes; // Instruction length (1-3)
};

class Disassembler {
public:
  explicit Disassembler(std::shared_ptr<mmu::Mmunit> mmu);

  // Disassemble one instruction at addr. Returns the disassembled line.
  DisasmLine disassemble(uint16_t addr) const;

  // Disassemble n instructions starting from addr.
  std::vector<DisasmLine> disassembleN(uint16_t addr, int n) const;

private:
  std::shared_ptr<mmu::Mmunit> mmu_;

  struct OpcodeInfo {
    const char *fmt;   // Format string (e.g., "LD B, $%02X")
    uint8_t operands;  // Number of operand bytes (0, 1, or 2)
  };

  static const OpcodeInfo BASE_OPCODES[256];
  static const OpcodeInfo CB_OPCODES[256];
};

} // namespace debugger
