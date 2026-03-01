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

private:
  std::shared_ptr<input::Joypad> joypad_;

  void handleKey(SDL_Keycode key, bool pressed);
};

} // namespace platform
