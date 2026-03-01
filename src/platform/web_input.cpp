#include "platform/web_input.hpp"
#include <SDL.h>

namespace platform {

WebInput::WebInput(std::shared_ptr<input::Joypad> joypad) : joypad_(joypad) {
    if (SDL_Init(SDL_INIT_EVENTS) < 0) {
        throw std::runtime_error(std::string("SDL_Init EVENTS failed: ") + SDL_GetError());
    }
}

WebInput::~WebInput() {
    SDL_QuitSubSystem(SDL_INIT_EVENTS);
}

bool WebInput::poll() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                return false;
            case SDL_KEYDOWN:
            case SDL_KEYUP:
                bool pressed = (event.type == SDL_KEYDOWN);
                switch (event.key.keysym.sym) {
                    case SDLK_RIGHT: joypad_->setButtonState(input::JoypadButton::Right, pressed); break;
                    case SDLK_LEFT:  joypad_->setButtonState(input::JoypadButton::Left, pressed); break;
                    case SDLK_UP:    joypad_->setButtonState(input::JoypadButton::Up, pressed); break;
                    case SDLK_DOWN:  joypad_->setButtonState(input::JoypadButton::Down, pressed); break;
                    case SDLK_z:     joypad_->setButtonState(input::JoypadButton::A, pressed); break;
                    case SDLK_x:     joypad_->setButtonState(input::JoypadButton::B, pressed); break;
                    case SDLK_RETURN: joypad_->setButtonState(input::JoypadButton::Start, pressed); break;
                    case SDLK_BACKSPACE: joypad_->setButtonState(input::JoypadButton::Select, pressed); break;
                }
                break;
        }
    }
    return true;
}

} // namespace platform