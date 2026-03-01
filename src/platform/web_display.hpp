#pragma once

#include "ppu/ppu.hpp"
#include <cstdint>
#include <array>
#include <SDL.h>

namespace platform {

class WebDisplay {
public:
    WebDisplay(int scale = 3);
    ~WebDisplay();

    WebDisplay(const WebDisplay &) = delete;
    WebDisplay &operator=(const WebDisplay &) = delete;

    // Render a framebuffer to the screen
    void render(const std::array<ppu::Color, ppu::SCREEN_WIDTH * ppu::SCREEN_HEIGHT> &fb);

private:
    int scale_;
    SDL_Window *window_ = nullptr;
    SDL_Renderer *renderer_ = nullptr;
    SDL_Texture *texture_ = nullptr;
};

} // namespace platform