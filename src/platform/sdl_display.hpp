#pragma once

#include "ppu/ppu.hpp"
#include <SDL2/SDL.h>
#include <cstdint>
#include <string>

namespace platform {

class SdlDisplay {
public:
  SdlDisplay(int scale = 3);
  ~SdlDisplay();

  SdlDisplay(const SdlDisplay &) = delete;
  SdlDisplay &operator=(const SdlDisplay &) = delete;

  // Render a framebuffer to the screen
  void render(const std::array<ppu::Color, ppu::SCREEN_WIDTH * ppu::SCREEN_HEIGHT> &fb);

  // Frame timing: delay to maintain ~60fps
  void syncFrame();

private:
  SDL_Window *window_ = nullptr;
  SDL_Renderer *renderer_ = nullptr;
  SDL_Texture *texture_ = nullptr;
  int scale_;
  uint32_t last_frame_time_ = 0;
};

} // namespace platform
