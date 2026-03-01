#include "platform/web_display.hpp"
#include <stdexcept>
#include <SDL.h>

namespace platform {

WebDisplay::WebDisplay(int scale) : scale_(scale) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }

    window_ = SDL_CreateWindow("XBoy",
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               ppu::SCREEN_WIDTH * scale_, ppu::SCREEN_HEIGHT * scale_,
                               SDL_WINDOW_SHOWN);
    if (!window_) {
        throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
    }

    renderer_ = SDL_CreateRenderer(window_, -1,
                                   SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) {
        throw std::runtime_error(std::string("SDL_CreateRenderer failed: ") + SDL_GetError());
    }

    texture_ = SDL_CreateTexture(renderer_,
                                 SDL_PIXELFORMAT_RGBA32,
                                 SDL_TEXTUREACCESS_STREAMING,
                                 ppu::SCREEN_WIDTH, ppu::SCREEN_HEIGHT);
    if (!texture_) {
        throw std::runtime_error(std::string("SDL_CreateTexture failed: ") + SDL_GetError());
    }
}

WebDisplay::~WebDisplay() {
    if (texture_) SDL_DestroyTexture(texture_);
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
    SDL_Quit();
}

void WebDisplay::render(
    const std::array<ppu::Color, ppu::SCREEN_WIDTH * ppu::SCREEN_HEIGHT> &fb) {
    SDL_UpdateTexture(texture_, nullptr, fb.data(),
                      ppu::SCREEN_WIDTH * sizeof(ppu::Color));
    SDL_RenderClear(renderer_);
    SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
}

} // namespace platform