#include "cpu/cpu.hpp"
#include "state/serializable.hpp"

namespace cpu {

Cpu::Cpu(std::shared_ptr<mmu::Mmunit> mmu) : mmu_(mmu) { regs_.initDMG(); }

uint8_t Cpu::step() {
  // Handle EI delayed effect
  if (ei_pending_) {
    ime_ = true;
    ei_pending_ = false;
  }

  if (halted_) {
    return 4; // HALT consumes 4 cycles per step while waiting
  }

  uint8_t opcode = fetchByte();
  return executeOp(opcode);
}

uint8_t Cpu::handleInterrupts() {
  uint8_t ie = read(0xFFFF);
  uint8_t if_reg = read(0xFF0F);
  uint8_t pending = ie & if_reg & 0x1F;

  if (pending == 0) {
    return 0;
  }

  // Any pending interrupt wakes from HALT even if IME is disabled
  halted_ = false;

  if (!ime_) {
    return 0;
  }

  // Service highest priority interrupt
  ime_ = false;

  // Priority: VBlank > LCD STAT > Timer > Serial > Joypad
  static const uint16_t vectors[] = {0x0040, 0x0048, 0x0050, 0x0058, 0x0060};

  for (int i = 0; i < 5; i++) {
    if (pending & (1 << i)) {
      // Clear interrupt flag
      write(0xFF0F, if_reg & ~(1 << i));
      // Push current PC and jump to interrupt vector
      pushWord(regs_.pc);
      regs_.pc = vectors[i];
      return 20; // Interrupt dispatch takes 20 cycles
    }
  }

  return 0;
}

// --- Memory access helpers ---

uint8_t Cpu::fetchByte() { return read(regs_.pc++); }

uint16_t Cpu::fetchWord() {
  uint16_t lo = read(regs_.pc++);
  uint16_t hi = read(regs_.pc++);
  return (hi << 8) | lo;
}

uint8_t Cpu::read(uint16_t addr) const { return mmu_->get(addr); }
void Cpu::write(uint16_t addr, uint8_t val) { mmu_->set(addr, val); }

uint16_t Cpu::readWord(uint16_t addr) const {
  return mmu_->getWord(addr);
}
void Cpu::writeWord(uint16_t addr, uint16_t val) {
  mmu_->setWord(addr, val);
}

void Cpu::pushWord(uint16_t val) {
  regs_.sp -= 2;
  writeWord(regs_.sp, val);
}

uint16_t Cpu::popWord() {
  uint16_t val = readWord(regs_.sp);
  regs_.sp += 2;
  return val;
}

// --- ALU operations ---

void Cpu::alu_add(uint8_t val) {
  uint16_t result = regs_.a + val;
  regs_.setFlags((result & 0xFF) == 0, false,
                 ((regs_.a & 0x0F) + (val & 0x0F)) > 0x0F, result > 0xFF);
  regs_.a = static_cast<uint8_t>(result);
}

void Cpu::alu_adc(uint8_t val) {
  uint8_t carry = regs_.flagC() ? 1 : 0;
  uint16_t result = regs_.a + val + carry;
  regs_.setFlags((result & 0xFF) == 0, false,
                 ((regs_.a & 0x0F) + (val & 0x0F) + carry) > 0x0F,
                 result > 0xFF);
  regs_.a = static_cast<uint8_t>(result);
}

void Cpu::alu_sub(uint8_t val) {
  uint16_t result = regs_.a - val;
  regs_.setFlags((result & 0xFF) == 0, true,
                 (regs_.a & 0x0F) < (val & 0x0F), regs_.a < val);
  regs_.a = static_cast<uint8_t>(result);
}

void Cpu::alu_sbc(uint8_t val) {
  uint8_t carry = regs_.flagC() ? 1 : 0;
  int result = regs_.a - val - carry;
  regs_.setFlags((result & 0xFF) == 0, true,
                 ((regs_.a & 0x0F) - (val & 0x0F) - carry) < 0,
                 result < 0);
  regs_.a = static_cast<uint8_t>(result);
}

void Cpu::alu_and(uint8_t val) {
  regs_.a &= val;
  regs_.setFlags(regs_.a == 0, false, true, false);
}

void Cpu::alu_or(uint8_t val) {
  regs_.a |= val;
  regs_.setFlags(regs_.a == 0, false, false, false);
}

void Cpu::alu_xor(uint8_t val) {
  regs_.a ^= val;
  regs_.setFlags(regs_.a == 0, false, false, false);
}

void Cpu::alu_cp(uint8_t val) {
  regs_.setFlags(regs_.a == val, true, (regs_.a & 0x0F) < (val & 0x0F),
                 regs_.a < val);
}

uint8_t Cpu::alu_inc(uint8_t val) {
  uint8_t result = val + 1;
  regs_.setFlagZ(result == 0);
  regs_.setFlagN(false);
  regs_.setFlagH((val & 0x0F) == 0x0F);
  // C flag not affected
  return result;
}

uint8_t Cpu::alu_dec(uint8_t val) {
  uint8_t result = val - 1;
  regs_.setFlagZ(result == 0);
  regs_.setFlagN(true);
  regs_.setFlagH((val & 0x0F) == 0x00);
  // C flag not affected
  return result;
}

void Cpu::alu_add_hl(uint16_t val) {
  uint32_t result = regs_.hl() + val;
  regs_.setFlagN(false);
  regs_.setFlagH(((regs_.hl() & 0x0FFF) + (val & 0x0FFF)) > 0x0FFF);
  regs_.setFlagC(result > 0xFFFF);
  regs_.setHL(static_cast<uint16_t>(result));
}

uint16_t Cpu::alu_add_sp(int8_t val) {
  uint16_t sp = regs_.sp;
  uint16_t result = static_cast<uint16_t>(sp + val);
  regs_.setFlags(false, false, ((sp & 0x0F) + (val & 0x0F)) > 0x0F,
                 ((sp & 0xFF) + (val & 0xFF)) > 0xFF);
  return result;
}

void Cpu::alu_daa() {
  int a = regs_.a;
  if (!regs_.flagN()) {
    if (regs_.flagH() || (a & 0x0F) > 9) {
      a += 0x06;
    }
    if (regs_.flagC() || a > 0x9F) {
      a += 0x60;
    }
  } else {
    if (regs_.flagH()) {
      a = (a - 0x06) & 0xFF;
    }
    if (regs_.flagC()) {
      a -= 0x60;
    }
  }
  regs_.setFlagH(false);
  if ((a & 0x100) != 0) {
    regs_.setFlagC(true);
  }
  regs_.a = static_cast<uint8_t>(a & 0xFF);
  regs_.setFlagZ(regs_.a == 0);
}

// --- CB-prefix operations ---

uint8_t Cpu::cb_rlc(uint8_t val) {
  uint8_t carry = (val >> 7) & 1;
  uint8_t result = (val << 1) | carry;
  regs_.setFlags(result == 0, false, false, carry);
  return result;
}

uint8_t Cpu::cb_rrc(uint8_t val) {
  uint8_t carry = val & 1;
  uint8_t result = (val >> 1) | (carry << 7);
  regs_.setFlags(result == 0, false, false, carry);
  return result;
}

uint8_t Cpu::cb_rl(uint8_t val) {
  uint8_t old_carry = regs_.flagC() ? 1 : 0;
  uint8_t new_carry = (val >> 7) & 1;
  uint8_t result = (val << 1) | old_carry;
  regs_.setFlags(result == 0, false, false, new_carry);
  return result;
}

uint8_t Cpu::cb_rr(uint8_t val) {
  uint8_t old_carry = regs_.flagC() ? 1 : 0;
  uint8_t new_carry = val & 1;
  uint8_t result = (val >> 1) | (old_carry << 7);
  regs_.setFlags(result == 0, false, false, new_carry);
  return result;
}

uint8_t Cpu::cb_sla(uint8_t val) {
  uint8_t carry = (val >> 7) & 1;
  uint8_t result = val << 1;
  regs_.setFlags(result == 0, false, false, carry);
  return result;
}

uint8_t Cpu::cb_sra(uint8_t val) {
  uint8_t carry = val & 1;
  uint8_t result = (val >> 1) | (val & 0x80); // preserve bit 7
  regs_.setFlags(result == 0, false, false, carry);
  return result;
}

uint8_t Cpu::cb_swap(uint8_t val) {
  uint8_t result = ((val & 0x0F) << 4) | ((val & 0xF0) >> 4);
  regs_.setFlags(result == 0, false, false, false);
  return result;
}

uint8_t Cpu::cb_srl(uint8_t val) {
  uint8_t carry = val & 1;
  uint8_t result = val >> 1;
  regs_.setFlags(result == 0, false, false, carry);
  return result;
}

void Cpu::cb_bit(uint8_t bit, uint8_t val) {
  regs_.setFlagZ(!((val >> bit) & 1));
  regs_.setFlagN(false);
  regs_.setFlagH(true);
  // C not affected
}

// --- Save state serialization ---

void Cpu::serialize(std::vector<uint8_t> &buf) const {
  state::write_u8(buf, regs_.a);
  state::write_u8(buf, regs_.f);
  state::write_u8(buf, regs_.b);
  state::write_u8(buf, regs_.c);
  state::write_u8(buf, regs_.d);
  state::write_u8(buf, regs_.e);
  state::write_u8(buf, regs_.h);
  state::write_u8(buf, regs_.l);
  state::write_u16(buf, regs_.sp);
  state::write_u16(buf, regs_.pc);
  state::write_bool(buf, ime_);
  state::write_bool(buf, ei_pending_);
  state::write_bool(buf, halted_);
  state::write_bool(buf, stopped_);
}

void Cpu::deserialize(const uint8_t *data, size_t &pos) {
  regs_.a = state::read_u8(data, pos);
  regs_.f = state::read_u8(data, pos);
  regs_.b = state::read_u8(data, pos);
  regs_.c = state::read_u8(data, pos);
  regs_.d = state::read_u8(data, pos);
  regs_.e = state::read_u8(data, pos);
  regs_.h = state::read_u8(data, pos);
  regs_.l = state::read_u8(data, pos);
  regs_.sp = state::read_u16(data, pos);
  regs_.pc = state::read_u16(data, pos);
  ime_ = state::read_bool(data, pos);
  ei_pending_ = state::read_bool(data, pos);
  halted_ = state::read_bool(data, pos);
  stopped_ = state::read_bool(data, pos);
}

} // namespace cpu
