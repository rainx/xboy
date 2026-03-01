#pragma once

#include "input/joypad.hpp"
#include <SDL2/SDL.h>
#include <memory>

namespace platform {

class SdlInput {
public:
  SdlInput(std::shared_ptr<input::Joypad> joypad);

  // Process SDL events. Returns false if quit was requested.
  bool poll();

  // Check and clear special key flags
  bool saveRequested() { bool r = save_requested_; save_requested_ = false; return r; }
  bool loadRequested() { bool r = load_requested_; load_requested_ = false; return r; }
  bool debugToggleRequested() { bool r = debug_toggle_; debug_toggle_ = false; return r; }

private:
  std::shared_ptr<input::Joypad> joypad_;

  bool save_requested_ = false;
  bool load_requested_ = false;
  bool debug_toggle_ = false;

  void handleKey(SDL_Keycode key, bool pressed);
};

} // namespace platform
