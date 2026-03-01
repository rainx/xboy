#pragma once

#include "mmu/mmunit.hpp"
#include <cstdint>
#include <memory>
#include <vector>

namespace input {

// Game Boy Joypad (P1/JOYP register at 0xFF00)
//
// Bit 5: Select action buttons   (0 = selected)
// Bit 4: Select direction buttons (0 = selected)
// Bit 3: Down  or Start  (0 = pressed)
// Bit 2: Up    or Select (0 = pressed)
// Bit 1: Left  or B      (0 = pressed)
// Bit 0: Right or A      (0 = pressed)
//
// Note: Bits are active-low (0 = pressed/selected)

enum class Button : uint8_t {
  Right = 0,
  Left = 1,
  Up = 2,
  Down = 3,
  A = 4,
  B = 5,
  Select = 6,
  Start = 7,
};

class Joypad {
public:
  Joypad(std::shared_ptr<mmu::Mmunit> mmu);

  void press(Button btn);
  void release(Button btn);

  // Called by MMU/main loop to update the JOYP register based on
  // the select bits written by the game
  void update();

  // Save state serialization
  void serialize(std::vector<uint8_t> &buf) const;
  void deserialize(const uint8_t *data, size_t &pos);

private:
  std::shared_ptr<mmu::Mmunit> mmu_;

  // Button state: bit set = pressed
  // Bits 0-3: Right, Left, Up, Down (direction)
  // Bits 4-7: A, B, Select, Start (action)
  uint8_t button_state_ = 0;
};

} // namespace input
