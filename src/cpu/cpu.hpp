#pragma once

#include "cpu/registers.hpp"
#include "mmu/mmunit.hpp"

#include <cstdint>
#include <memory>

namespace cpu {

class Cpu {
public:
  Cpu(std::shared_ptr<mmu::Mmunit> mmu);

  // Execute one instruction, return cycles consumed (in T-cycles, 4.19MHz)
  uint8_t step();

  // Accessors
  Registers &regs() { return regs_; }
  const Registers &regs() const { return regs_; }

  bool isHalted() const { return halted_; }
  bool isStopped() const { return stopped_; }
  bool ime() const { return ime_; }
  void setIME(bool v) { ime_ = v; }

  // Handle interrupts — called after step() in the main loop
  // Returns cycles consumed by interrupt dispatch (0 if no interrupt)
  uint8_t handleInterrupts();

private:
  // Fetch byte at PC and advance PC
  uint8_t fetchByte();
  // Fetch word (little-endian) at PC and advance PC by 2
  uint16_t fetchWord();

  // Read/write helpers
  uint8_t read(uint16_t addr) const;
  void write(uint16_t addr, uint8_t val);
  uint16_t readWord(uint16_t addr) const;
  void writeWord(uint16_t addr, uint16_t val);

  // Stack operations
  void pushWord(uint16_t val);
  uint16_t popWord();

  // Execute base opcode (0x00-0xFF)
  uint8_t executeOp(uint8_t opcode);
  // Execute CB-prefix opcode
  uint8_t executeCB(uint8_t opcode);

  // ALU operations
  void alu_add(uint8_t val);
  void alu_adc(uint8_t val);
  void alu_sub(uint8_t val);
  void alu_sbc(uint8_t val);
  void alu_and(uint8_t val);
  void alu_or(uint8_t val);
  void alu_xor(uint8_t val);
  void alu_cp(uint8_t val);
  uint8_t alu_inc(uint8_t val);
  uint8_t alu_dec(uint8_t val);
  void alu_add_hl(uint16_t val);
  uint16_t alu_add_sp(int8_t val);
  void alu_daa();

  // CB-prefix operations
  uint8_t cb_rlc(uint8_t val);
  uint8_t cb_rrc(uint8_t val);
  uint8_t cb_rl(uint8_t val);
  uint8_t cb_rr(uint8_t val);
  uint8_t cb_sla(uint8_t val);
  uint8_t cb_sra(uint8_t val);
  uint8_t cb_swap(uint8_t val);
  uint8_t cb_srl(uint8_t val);
  void cb_bit(uint8_t bit, uint8_t val);

  Registers regs_;
  std::shared_ptr<mmu::Mmunit> mmu_;

  bool ime_ = false;    // Interrupt Master Enable
  bool ei_pending_ = false; // EI takes effect after next instruction
  bool halted_ = false;
  bool stopped_ = false;
};

} // namespace cpu
