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
    // On the web, keyboard input is handled by JavaScript via setButtonState().
    // SDL_PollEvent would intercept browser key events and conflict with JS handlers.
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // Drain SDL event queue but don't process keyboard events
    }
    return true;
}

} // namespace platform