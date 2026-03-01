#include "platform/sdl_input.hpp"

namespace platform {

SdlInput::SdlInput(std::shared_ptr<input::Joypad> joypad)
    : joypad_(joypad) {}

bool SdlInput::poll() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_QUIT:
      return false;

    case SDL_KEYDOWN:
      if (event.key.keysym.sym == SDLK_ESCAPE) {
        return false;
      }
      handleKey(event.key.keysym.sym, true);
      break;

    case SDL_KEYUP:
      handleKey(event.key.keysym.sym, false);
      break;
    }
  }
  return true;
}

void SdlInput::handleKey(SDL_Keycode key, bool pressed) {
  // Keyboard mapping:
  //   Arrow keys → D-Pad
  //   Z → A, X → B
  //   Enter → Start, Backspace → Select
  input::Button btn;

  switch (key) {
  case SDLK_RIGHT: btn = input::Button::Right; break;
  case SDLK_LEFT:  btn = input::Button::Left; break;
  case SDLK_UP:    btn = input::Button::Up; break;
  case SDLK_DOWN:  btn = input::Button::Down; break;
  case SDLK_z:     btn = input::Button::A; break;
  case SDLK_x:     btn = input::Button::B; break;
  case SDLK_RETURN:    btn = input::Button::Start; break;
  case SDLK_BACKSPACE: btn = input::Button::Select; break;
  default: return;
  }

  if (pressed) {
    joypad_->press(btn);
  } else {
    joypad_->release(btn);
  }
}

} // namespace platform
