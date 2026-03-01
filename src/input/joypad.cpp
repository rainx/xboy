#include "input/joypad.hpp"
#include "cpu/interrupts.hpp"
#include "state/serializable.hpp"

namespace input {

Joypad::Joypad(std::shared_ptr<mmu::Mmunit> mmu) : mmu_(mmu) {}

void Joypad::press(Button btn) {
  uint8_t old_state = button_state_;
  button_state_ |= (1 << static_cast<uint8_t>(btn));

  // If a button was newly pressed, request joypad interrupt
  if (button_state_ != old_state) {
    uint8_t if_reg = mmu_->get(0xFF0F);
    mmu_->set(0xFF0F, if_reg | cpu::InterruptFlag::Joypad);
  }

  update();
}

void Joypad::release(Button btn) {
  button_state_ &= ~(1 << static_cast<uint8_t>(btn));
  update();
}

void Joypad::update() {
  uint8_t joyp = mmu_->get(0xFF00);

  // The upper nibble (bits 4-5) are select lines written by the game
  // We compute the lower nibble based on which group is selected
  uint8_t result = joyp & 0x30; // Keep select bits

  if (!(result & 0x10)) {
    // Direction buttons selected (bit 4 = 0)
    // Bits 0-3 map to Right, Left, Up, Down
    uint8_t dirs = button_state_ & 0x0F;
    result |= (~dirs) & 0x0F; // Active-low
  }

  if (!(result & 0x20)) {
    // Action buttons selected (bit 5 = 0)
    // Bits 4-7 of button_state_ map to A, B, Select, Start
    uint8_t actions = (button_state_ >> 4) & 0x0F;
    result |= (~actions) & 0x0F; // Active-low
  }

  // If neither selected, all bits high
  if ((result & 0x30) == 0x30) {
    result |= 0x0F;
  }

  mmu_->set(0xFF00, result);
}

void Joypad::serialize(std::vector<uint8_t> &buf) const {
  state::write_u8(buf, button_state_);
}

void Joypad::deserialize(const uint8_t *data, size_t &pos) {
  button_state_ = state::read_u8(data, pos);
}

} // namespace input
