#include "cpu/cpu.hpp"

namespace cpu {

// Execute a single base opcode. Returns T-cycles consumed.
uint8_t Cpu::executeOp(uint8_t opcode) {
  switch (opcode) {

  // ============================================================
  // 0x00 - 0x0F
  // ============================================================

  case 0x00: // NOP
    return 4;

  case 0x01: // LD BC, d16
    regs_.setBC(fetchWord());
    return 12;

  case 0x02: // LD (BC), A
    write(regs_.bc(), regs_.a);
    return 8;

  case 0x03: // INC BC
    regs_.setBC(regs_.bc() + 1);
    return 8;

  case 0x04: // INC B
    regs_.b = alu_inc(regs_.b);
    return 4;

  case 0x05: // DEC B
    regs_.b = alu_dec(regs_.b);
    return 4;

  case 0x06: // LD B, d8
    regs_.b = fetchByte();
    return 8;

  case 0x07: { // RLCA
    uint8_t carry = (regs_.a >> 7) & 1;
    regs_.a = (regs_.a << 1) | carry;
    regs_.setFlags(false, false, false, carry);
    return 4;
  }

  case 0x08: { // LD (a16), SP
    uint16_t addr = fetchWord();
    writeWord(addr, regs_.sp);
    return 20;
  }

  case 0x09: // ADD HL, BC
    alu_add_hl(regs_.bc());
    return 8;

  case 0x0A: // LD A, (BC)
    regs_.a = read(regs_.bc());
    return 8;

  case 0x0B: // DEC BC
    regs_.setBC(regs_.bc() - 1);
    return 8;

  case 0x0C: // INC C
    regs_.c = alu_inc(regs_.c);
    return 4;

  case 0x0D: // DEC C
    regs_.c = alu_dec(regs_.c);
    return 4;

  case 0x0E: // LD C, d8
    regs_.c = fetchByte();
    return 8;

  case 0x0F: { // RRCA
    uint8_t carry = regs_.a & 1;
    regs_.a = (regs_.a >> 1) | (carry << 7);
    regs_.setFlags(false, false, false, carry);
    return 4;
  }

  // ============================================================
  // 0x10 - 0x1F
  // ============================================================

  case 0x10: // STOP
    fetchByte(); // consume extra byte
    stopped_ = true;
    return 4;

  case 0x11: // LD DE, d16
    regs_.setDE(fetchWord());
    return 12;

  case 0x12: // LD (DE), A
    write(regs_.de(), regs_.a);
    return 8;

  case 0x13: // INC DE
    regs_.setDE(regs_.de() + 1);
    return 8;

  case 0x14: // INC D
    regs_.d = alu_inc(regs_.d);
    return 4;

  case 0x15: // DEC D
    regs_.d = alu_dec(regs_.d);
    return 4;

  case 0x16: // LD D, d8
    regs_.d = fetchByte();
    return 8;

  case 0x17: { // RLA
    uint8_t old_carry = regs_.flagC() ? 1 : 0;
    uint8_t new_carry = (regs_.a >> 7) & 1;
    regs_.a = (regs_.a << 1) | old_carry;
    regs_.setFlags(false, false, false, new_carry);
    return 4;
  }

  case 0x18: { // JR r8
    int8_t offset = static_cast<int8_t>(fetchByte());
    regs_.pc += offset;
    return 12;
  }

  case 0x19: // ADD HL, DE
    alu_add_hl(regs_.de());
    return 8;

  case 0x1A: // LD A, (DE)
    regs_.a = read(regs_.de());
    return 8;

  case 0x1B: // DEC DE
    regs_.setDE(regs_.de() - 1);
    return 8;

  case 0x1C: // INC E
    regs_.e = alu_inc(regs_.e);
    return 4;

  case 0x1D: // DEC E
    regs_.e = alu_dec(regs_.e);
    return 4;

  case 0x1E: // LD E, d8
    regs_.e = fetchByte();
    return 8;

  case 0x1F: { // RRA
    uint8_t old_carry = regs_.flagC() ? 1 : 0;
    uint8_t new_carry = regs_.a & 1;
    regs_.a = (regs_.a >> 1) | (old_carry << 7);
    regs_.setFlags(false, false, false, new_carry);
    return 4;
  }

  // ============================================================
  // 0x20 - 0x2F
  // ============================================================

  case 0x20: { // JR NZ, r8
    int8_t offset = static_cast<int8_t>(fetchByte());
    if (!regs_.flagZ()) {
      regs_.pc += offset;
      return 12;
    }
    return 8;
  }

  case 0x21: // LD HL, d16
    regs_.setHL(fetchWord());
    return 12;

  case 0x22: // LD (HL+), A
    write(regs_.hl(), regs_.a);
    regs_.setHL(regs_.hl() + 1);
    return 8;

  case 0x23: // INC HL
    regs_.setHL(regs_.hl() + 1);
    return 8;

  case 0x24: // INC H
    regs_.h = alu_inc(regs_.h);
    return 4;

  case 0x25: // DEC H
    regs_.h = alu_dec(regs_.h);
    return 4;

  case 0x26: // LD H, d8
    regs_.h = fetchByte();
    return 8;

  case 0x27: // DAA
    alu_daa();
    return 4;

  case 0x28: { // JR Z, r8
    int8_t offset = static_cast<int8_t>(fetchByte());
    if (regs_.flagZ()) {
      regs_.pc += offset;
      return 12;
    }
    return 8;
  }

  case 0x29: // ADD HL, HL
    alu_add_hl(regs_.hl());
    return 8;

  case 0x2A: // LD A, (HL+)
    regs_.a = read(regs_.hl());
    regs_.setHL(regs_.hl() + 1);
    return 8;

  case 0x2B: // DEC HL
    regs_.setHL(regs_.hl() - 1);
    return 8;

  case 0x2C: // INC L
    regs_.l = alu_inc(regs_.l);
    return 4;

  case 0x2D: // DEC L
    regs_.l = alu_dec(regs_.l);
    return 4;

  case 0x2E: // LD L, d8
    regs_.l = fetchByte();
    return 8;

  case 0x2F: // CPL
    regs_.a = ~regs_.a;
    regs_.setFlagN(true);
    regs_.setFlagH(true);
    return 4;

  // ============================================================
  // 0x30 - 0x3F
  // ============================================================

  case 0x30: { // JR NC, r8
    int8_t offset = static_cast<int8_t>(fetchByte());
    if (!regs_.flagC()) {
      regs_.pc += offset;
      return 12;
    }
    return 8;
  }

  case 0x31: // LD SP, d16
    regs_.sp = fetchWord();
    return 12;

  case 0x32: // LD (HL-), A
    write(regs_.hl(), regs_.a);
    regs_.setHL(regs_.hl() - 1);
    return 8;

  case 0x33: // INC SP
    regs_.sp++;
    return 8;

  case 0x34: { // INC (HL)
    uint8_t val = read(regs_.hl());
    write(regs_.hl(), alu_inc(val));
    return 12;
  }

  case 0x35: { // DEC (HL)
    uint8_t val = read(regs_.hl());
    write(regs_.hl(), alu_dec(val));
    return 12;
  }

  case 0x36: // LD (HL), d8
    write(regs_.hl(), fetchByte());
    return 12;

  case 0x37: // SCF
    regs_.setFlagN(false);
    regs_.setFlagH(false);
    regs_.setFlagC(true);
    return 4;

  case 0x38: { // JR C, r8
    int8_t offset = static_cast<int8_t>(fetchByte());
    if (regs_.flagC()) {
      regs_.pc += offset;
      return 12;
    }
    return 8;
  }

  case 0x39: // ADD HL, SP
    alu_add_hl(regs_.sp);
    return 8;

  case 0x3A: // LD A, (HL-)
    regs_.a = read(regs_.hl());
    regs_.setHL(regs_.hl() - 1);
    return 8;

  case 0x3B: // DEC SP
    regs_.sp--;
    return 8;

  case 0x3C: // INC A
    regs_.a = alu_inc(regs_.a);
    return 4;

  case 0x3D: // DEC A
    regs_.a = alu_dec(regs_.a);
    return 4;

  case 0x3E: // LD A, d8
    regs_.a = fetchByte();
    return 8;

  case 0x3F: // CCF
    regs_.setFlagN(false);
    regs_.setFlagH(false);
    regs_.setFlagC(!regs_.flagC());
    return 4;

  // ============================================================
  // 0x40 - 0x7F: LD register-to-register block
  // ============================================================

  // LD B, r
  case 0x40: return 4; // LD B, B (nop)
  case 0x41: regs_.b = regs_.c; return 4;
  case 0x42: regs_.b = regs_.d; return 4;
  case 0x43: regs_.b = regs_.e; return 4;
  case 0x44: regs_.b = regs_.h; return 4;
  case 0x45: regs_.b = regs_.l; return 4;
  case 0x46: regs_.b = read(regs_.hl()); return 8;
  case 0x47: regs_.b = regs_.a; return 4;

  // LD C, r
  case 0x48: regs_.c = regs_.b; return 4;
  case 0x49: return 4; // LD C, C (nop)
  case 0x4A: regs_.c = regs_.d; return 4;
  case 0x4B: regs_.c = regs_.e; return 4;
  case 0x4C: regs_.c = regs_.h; return 4;
  case 0x4D: regs_.c = regs_.l; return 4;
  case 0x4E: regs_.c = read(regs_.hl()); return 8;
  case 0x4F: regs_.c = regs_.a; return 4;

  // LD D, r
  case 0x50: regs_.d = regs_.b; return 4;
  case 0x51: regs_.d = regs_.c; return 4;
  case 0x52: return 4; // LD D, D (nop)
  case 0x53: regs_.d = regs_.e; return 4;
  case 0x54: regs_.d = regs_.h; return 4;
  case 0x55: regs_.d = regs_.l; return 4;
  case 0x56: regs_.d = read(regs_.hl()); return 8;
  case 0x57: regs_.d = regs_.a; return 4;

  // LD E, r
  case 0x58: regs_.e = regs_.b; return 4;
  case 0x59: regs_.e = regs_.c; return 4;
  case 0x5A: regs_.e = regs_.d; return 4;
  case 0x5B: return 4; // LD E, E (nop)
  case 0x5C: regs_.e = regs_.h; return 4;
  case 0x5D: regs_.e = regs_.l; return 4;
  case 0x5E: regs_.e = read(regs_.hl()); return 8;
  case 0x5F: regs_.e = regs_.a; return 4;

  // LD H, r
  case 0x60: regs_.h = regs_.b; return 4;
  case 0x61: regs_.h = regs_.c; return 4;
  case 0x62: regs_.h = regs_.d; return 4;
  case 0x63: regs_.h = regs_.e; return 4;
  case 0x64: return 4; // LD H, H (nop)
  case 0x65: regs_.h = regs_.l; return 4;
  case 0x66: regs_.h = read(regs_.hl()); return 8;
  case 0x67: regs_.h = regs_.a; return 4;

  // LD L, r
  case 0x68: regs_.l = regs_.b; return 4;
  case 0x69: regs_.l = regs_.c; return 4;
  case 0x6A: regs_.l = regs_.d; return 4;
  case 0x6B: regs_.l = regs_.e; return 4;
  case 0x6C: regs_.l = regs_.h; return 4;
  case 0x6D: return 4; // LD L, L (nop)
  case 0x6E: regs_.l = read(regs_.hl()); return 8;
  case 0x6F: regs_.l = regs_.a; return 4;

  // LD (HL), r
  case 0x70: write(regs_.hl(), regs_.b); return 8;
  case 0x71: write(regs_.hl(), regs_.c); return 8;
  case 0x72: write(regs_.hl(), regs_.d); return 8;
  case 0x73: write(regs_.hl(), regs_.e); return 8;
  case 0x74: write(regs_.hl(), regs_.h); return 8;
  case 0x75: write(regs_.hl(), regs_.l); return 8;

  case 0x76: // HALT
    halted_ = true;
    return 4;

  case 0x77: write(regs_.hl(), regs_.a); return 8;

  // LD A, r
  case 0x78: regs_.a = regs_.b; return 4;
  case 0x79: regs_.a = regs_.c; return 4;
  case 0x7A: regs_.a = regs_.d; return 4;
  case 0x7B: regs_.a = regs_.e; return 4;
  case 0x7C: regs_.a = regs_.h; return 4;
  case 0x7D: regs_.a = regs_.l; return 4;
  case 0x7E: regs_.a = read(regs_.hl()); return 8;
  case 0x7F: return 4; // LD A, A (nop)

  // ============================================================
  // 0x80 - 0xBF: ALU operations with register operands
  // ============================================================

  // ADD A, r
  case 0x80: alu_add(regs_.b); return 4;
  case 0x81: alu_add(regs_.c); return 4;
  case 0x82: alu_add(regs_.d); return 4;
  case 0x83: alu_add(regs_.e); return 4;
  case 0x84: alu_add(regs_.h); return 4;
  case 0x85: alu_add(regs_.l); return 4;
  case 0x86: alu_add(read(regs_.hl())); return 8;
  case 0x87: alu_add(regs_.a); return 4;

  // ADC A, r
  case 0x88: alu_adc(regs_.b); return 4;
  case 0x89: alu_adc(regs_.c); return 4;
  case 0x8A: alu_adc(regs_.d); return 4;
  case 0x8B: alu_adc(regs_.e); return 4;
  case 0x8C: alu_adc(regs_.h); return 4;
  case 0x8D: alu_adc(regs_.l); return 4;
  case 0x8E: alu_adc(read(regs_.hl())); return 8;
  case 0x8F: alu_adc(regs_.a); return 4;

  // SUB r
  case 0x90: alu_sub(regs_.b); return 4;
  case 0x91: alu_sub(regs_.c); return 4;
  case 0x92: alu_sub(regs_.d); return 4;
  case 0x93: alu_sub(regs_.e); return 4;
  case 0x94: alu_sub(regs_.h); return 4;
  case 0x95: alu_sub(regs_.l); return 4;
  case 0x96: alu_sub(read(regs_.hl())); return 8;
  case 0x97: alu_sub(regs_.a); return 4;

  // SBC A, r
  case 0x98: alu_sbc(regs_.b); return 4;
  case 0x99: alu_sbc(regs_.c); return 4;
  case 0x9A: alu_sbc(regs_.d); return 4;
  case 0x9B: alu_sbc(regs_.e); return 4;
  case 0x9C: alu_sbc(regs_.h); return 4;
  case 0x9D: alu_sbc(regs_.l); return 4;
  case 0x9E: alu_sbc(read(regs_.hl())); return 8;
  case 0x9F: alu_sbc(regs_.a); return 4;

  // AND r
  case 0xA0: alu_and(regs_.b); return 4;
  case 0xA1: alu_and(regs_.c); return 4;
  case 0xA2: alu_and(regs_.d); return 4;
  case 0xA3: alu_and(regs_.e); return 4;
  case 0xA4: alu_and(regs_.h); return 4;
  case 0xA5: alu_and(regs_.l); return 4;
  case 0xA6: alu_and(read(regs_.hl())); return 8;
  case 0xA7: alu_and(regs_.a); return 4;

  // XOR r
  case 0xA8: alu_xor(regs_.b); return 4;
  case 0xA9: alu_xor(regs_.c); return 4;
  case 0xAA: alu_xor(regs_.d); return 4;
  case 0xAB: alu_xor(regs_.e); return 4;
  case 0xAC: alu_xor(regs_.h); return 4;
  case 0xAD: alu_xor(regs_.l); return 4;
  case 0xAE: alu_xor(read(regs_.hl())); return 8;
  case 0xAF: alu_xor(regs_.a); return 4;

  // OR r
  case 0xB0: alu_or(regs_.b); return 4;
  case 0xB1: alu_or(regs_.c); return 4;
  case 0xB2: alu_or(regs_.d); return 4;
  case 0xB3: alu_or(regs_.e); return 4;
  case 0xB4: alu_or(regs_.h); return 4;
  case 0xB5: alu_or(regs_.l); return 4;
  case 0xB6: alu_or(read(regs_.hl())); return 8;
  case 0xB7: alu_or(regs_.a); return 4;

  // CP r
  case 0xB8: alu_cp(regs_.b); return 4;
  case 0xB9: alu_cp(regs_.c); return 4;
  case 0xBA: alu_cp(regs_.d); return 4;
  case 0xBB: alu_cp(regs_.e); return 4;
  case 0xBC: alu_cp(regs_.h); return 4;
  case 0xBD: alu_cp(regs_.l); return 4;
  case 0xBE: alu_cp(read(regs_.hl())); return 8;
  case 0xBF: alu_cp(regs_.a); return 4;

  // ============================================================
  // 0xC0 - 0xFF: Control flow, stack, misc
  // ============================================================

  case 0xC0: // RET NZ
    if (!regs_.flagZ()) {
      regs_.pc = popWord();
      return 20;
    }
    return 8;

  case 0xC1: // POP BC
    regs_.setBC(popWord());
    return 12;

  case 0xC2: { // JP NZ, a16
    uint16_t addr = fetchWord();
    if (!regs_.flagZ()) {
      regs_.pc = addr;
      return 16;
    }
    return 12;
  }

  case 0xC3: // JP a16
    regs_.pc = fetchWord();
    return 16;

  case 0xC4: { // CALL NZ, a16
    uint16_t addr = fetchWord();
    if (!regs_.flagZ()) {
      pushWord(regs_.pc);
      regs_.pc = addr;
      return 24;
    }
    return 12;
  }

  case 0xC5: // PUSH BC
    pushWord(regs_.bc());
    return 16;

  case 0xC6: // ADD A, d8
    alu_add(fetchByte());
    return 8;

  case 0xC7: // RST 00H
    pushWord(regs_.pc);
    regs_.pc = 0x0000;
    return 16;

  case 0xC8: // RET Z
    if (regs_.flagZ()) {
      regs_.pc = popWord();
      return 20;
    }
    return 8;

  case 0xC9: // RET
    regs_.pc = popWord();
    return 16;

  case 0xCA: { // JP Z, a16
    uint16_t addr = fetchWord();
    if (regs_.flagZ()) {
      regs_.pc = addr;
      return 16;
    }
    return 12;
  }

  case 0xCB: { // CB prefix
    uint8_t cb_opcode = fetchByte();
    return executeCB(cb_opcode);
  }

  case 0xCC: { // CALL Z, a16
    uint16_t addr = fetchWord();
    if (regs_.flagZ()) {
      pushWord(regs_.pc);
      regs_.pc = addr;
      return 24;
    }
    return 12;
  }

  case 0xCD: { // CALL a16
    uint16_t addr = fetchWord();
    pushWord(regs_.pc);
    regs_.pc = addr;
    return 24;
  }

  case 0xCE: // ADC A, d8
    alu_adc(fetchByte());
    return 8;

  case 0xCF: // RST 08H
    pushWord(regs_.pc);
    regs_.pc = 0x0008;
    return 16;

  case 0xD0: // RET NC
    if (!regs_.flagC()) {
      regs_.pc = popWord();
      return 20;
    }
    return 8;

  case 0xD1: // POP DE
    regs_.setDE(popWord());
    return 12;

  case 0xD2: { // JP NC, a16
    uint16_t addr = fetchWord();
    if (!regs_.flagC()) {
      regs_.pc = addr;
      return 16;
    }
    return 12;
  }

  // 0xD3: Unused
  case 0xD3:
    return 4;

  case 0xD4: { // CALL NC, a16
    uint16_t addr = fetchWord();
    if (!regs_.flagC()) {
      pushWord(regs_.pc);
      regs_.pc = addr;
      return 24;
    }
    return 12;
  }

  case 0xD5: // PUSH DE
    pushWord(regs_.de());
    return 16;

  case 0xD6: // SUB d8
    alu_sub(fetchByte());
    return 8;

  case 0xD7: // RST 10H
    pushWord(regs_.pc);
    regs_.pc = 0x0010;
    return 16;

  case 0xD8: // RET C
    if (regs_.flagC()) {
      regs_.pc = popWord();
      return 20;
    }
    return 8;

  case 0xD9: // RETI
    regs_.pc = popWord();
    ime_ = true;
    return 16;

  case 0xDA: { // JP C, a16
    uint16_t addr = fetchWord();
    if (regs_.flagC()) {
      regs_.pc = addr;
      return 16;
    }
    return 12;
  }

  // 0xDB: Unused
  case 0xDB:
    return 4;

  case 0xDC: { // CALL C, a16
    uint16_t addr = fetchWord();
    if (regs_.flagC()) {
      pushWord(regs_.pc);
      regs_.pc = addr;
      return 24;
    }
    return 12;
  }

  // 0xDD: Unused
  case 0xDD:
    return 4;

  case 0xDE: // SBC A, d8
    alu_sbc(fetchByte());
    return 8;

  case 0xDF: // RST 18H
    pushWord(regs_.pc);
    regs_.pc = 0x0018;
    return 16;

  case 0xE0: { // LDH (a8), A — LD (0xFF00+a8), A
    uint8_t offset = fetchByte();
    write(0xFF00 + offset, regs_.a);
    return 12;
  }

  case 0xE1: // POP HL
    regs_.setHL(popWord());
    return 12;

  case 0xE2: // LD (C), A — LD (0xFF00+C), A
    write(0xFF00 + regs_.c, regs_.a);
    return 8;

  // 0xE3, 0xE4: Unused
  case 0xE3:
  case 0xE4:
    return 4;

  case 0xE5: // PUSH HL
    pushWord(regs_.hl());
    return 16;

  case 0xE6: // AND d8
    alu_and(fetchByte());
    return 8;

  case 0xE7: // RST 20H
    pushWord(regs_.pc);
    regs_.pc = 0x0020;
    return 16;

  case 0xE8: { // ADD SP, r8
    int8_t val = static_cast<int8_t>(fetchByte());
    regs_.sp = alu_add_sp(val);
    return 16;
  }

  case 0xE9: // JP (HL) — actually JP HL (no dereference)
    regs_.pc = regs_.hl();
    return 4;

  case 0xEA: { // LD (a16), A
    uint16_t addr = fetchWord();
    write(addr, regs_.a);
    return 16;
  }

  // 0xEB, 0xEC, 0xED: Unused
  case 0xEB:
  case 0xEC:
  case 0xED:
    return 4;

  case 0xEE: // XOR d8
    alu_xor(fetchByte());
    return 8;

  case 0xEF: // RST 28H
    pushWord(regs_.pc);
    regs_.pc = 0x0028;
    return 16;

  case 0xF0: { // LDH A, (a8) — LD A, (0xFF00+a8)
    uint8_t offset = fetchByte();
    regs_.a = read(0xFF00 + offset);
    return 12;
  }

  case 0xF1: // POP AF
    regs_.setAF(popWord());
    return 12;

  case 0xF2: // LD A, (C) — LD A, (0xFF00+C)
    regs_.a = read(0xFF00 + regs_.c);
    return 8;

  case 0xF3: // DI
    ime_ = false;
    ei_pending_ = false;
    return 4;

  // 0xF4: Unused
  case 0xF4:
    return 4;

  case 0xF5: // PUSH AF
    pushWord(regs_.af());
    return 16;

  case 0xF6: // OR d8
    alu_or(fetchByte());
    return 8;

  case 0xF7: // RST 30H
    pushWord(regs_.pc);
    regs_.pc = 0x0030;
    return 16;

  case 0xF8: { // LD HL, SP+r8
    int8_t val = static_cast<int8_t>(fetchByte());
    regs_.setHL(alu_add_sp(val));
    return 12;
  }

  case 0xF9: // LD SP, HL
    regs_.sp = regs_.hl();
    return 8;

  case 0xFA: { // LD A, (a16)
    uint16_t addr = fetchWord();
    regs_.a = read(addr);
    return 16;
  }

  case 0xFB: // EI (takes effect after next instruction)
    ei_pending_ = true;
    return 4;

  // 0xFC, 0xFD: Unused
  case 0xFC:
  case 0xFD:
    return 4;

  case 0xFE: // CP d8
    alu_cp(fetchByte());
    return 8;

  case 0xFF: // RST 38H
    pushWord(regs_.pc);
    regs_.pc = 0x0038;
    return 16;

  default:
    return 4;
  }
}

// Execute CB-prefixed opcode. Returns T-cycles consumed.
uint8_t Cpu::executeCB(uint8_t opcode) {
  // CB opcodes follow a regular pattern:
  // Bits 7-6: operation group
  // Bits 5-3: bit number (for BIT/RES/SET) or sub-operation
  // Bits 2-0: register index (B=0, C=1, D=2, E=3, H=4, L=5, (HL)=6, A=7)

  auto getR = [&](uint8_t idx) -> uint8_t {
    switch (idx) {
    case 0: return regs_.b;
    case 1: return regs_.c;
    case 2: return regs_.d;
    case 3: return regs_.e;
    case 4: return regs_.h;
    case 5: return regs_.l;
    case 6: return read(regs_.hl());
    case 7: return regs_.a;
    default: return 0;
    }
  };

  auto setR = [&](uint8_t idx, uint8_t val) {
    switch (idx) {
    case 0: regs_.b = val; break;
    case 1: regs_.c = val; break;
    case 2: regs_.d = val; break;
    case 3: regs_.e = val; break;
    case 4: regs_.h = val; break;
    case 5: regs_.l = val; break;
    case 6: write(regs_.hl(), val); break;
    case 7: regs_.a = val; break;
    }
  };

  uint8_t reg_idx = opcode & 0x07;
  uint8_t bit = (opcode >> 3) & 0x07;
  uint8_t group = (opcode >> 6) & 0x03;
  bool is_hl = (reg_idx == 6);
  uint8_t cycles = is_hl ? 16 : 8;

  uint8_t val = getR(reg_idx);

  switch (group) {
  case 0: // Rotate/Shift operations (0x00-0x3F)
    switch (bit) {
    case 0: setR(reg_idx, cb_rlc(val)); break;
    case 1: setR(reg_idx, cb_rrc(val)); break;
    case 2: setR(reg_idx, cb_rl(val)); break;
    case 3: setR(reg_idx, cb_rr(val)); break;
    case 4: setR(reg_idx, cb_sla(val)); break;
    case 5: setR(reg_idx, cb_sra(val)); break;
    case 6: setR(reg_idx, cb_swap(val)); break;
    case 7: setR(reg_idx, cb_srl(val)); break;
    }
    break;

  case 1: // BIT b, r (0x40-0x7F)
    cb_bit(bit, val);
    if (is_hl)
      cycles = 12; // BIT b, (HL) is 12 cycles, not 16
    break;

  case 2: // RES b, r (0x80-0xBF)
    setR(reg_idx, val & ~(1 << bit));
    break;

  case 3: // SET b, r (0xC0-0xFF)
    setR(reg_idx, val | (1 << bit));
    break;
  }

  return cycles;
}

} // namespace cpu
