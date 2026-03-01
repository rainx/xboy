#include "ppu/ppu.hpp"
#include "cpu/interrupts.hpp"

namespace ppu {

// Classic Game Boy green palette
static const Color DMG_PALETTE[4] = {
    {224, 248, 208, 255}, // 0: Lightest
    {136, 192, 112, 255}, // 1: Light
    {52, 104, 86, 255},   // 2: Dark
    {8, 24, 32, 255},     // 3: Darkest
};

Ppu::Ppu(std::shared_ptr<mmu::Mmunit> mmu) : mmu_(mmu) {}

void Ppu::step(uint8_t cycles) {
  // If LCD is disabled, do nothing
  if (!(lcdc() & LcdcFlag::LcdEnable)) {
    return;
  }

  cycle_counter_ += cycles;

  switch (mode_) {
  case LcdMode::OamSearch: // Mode 2
    if (cycle_counter_ >= MODE2_CYCLES) {
      cycle_counter_ -= MODE2_CYCLES;
      mode_ = LcdMode::PixelTransfer;
      setStatMode(LcdMode::PixelTransfer);
    }
    break;

  case LcdMode::PixelTransfer: // Mode 3
    if (cycle_counter_ >= MODE3_CYCLES) {
      cycle_counter_ -= MODE3_CYCLES;
      renderScanline();
      enterHBlank();
    }
    break;

  case LcdMode::HBlank: // Mode 0
    if (cycle_counter_ >= MODE0_CYCLES) {
      cycle_counter_ -= MODE0_CYCLES;
      uint8_t current_ly = ly() + 1;
      setLY(current_ly);

      if (current_ly >= SCREEN_HEIGHT) {
        enterVBlank();
      } else {
        enterOamSearch();
      }
    }
    break;

  case LcdMode::VBlank: // Mode 1
    if (cycle_counter_ >= SCANLINE_CYCLES) {
      cycle_counter_ -= SCANLINE_CYCLES;
      uint8_t current_ly = ly() + 1;
      setLY(current_ly);

      if (current_ly >= TOTAL_SCANLINES) {
        setLY(0);
        window_line_ = 0;
        enterOamSearch();
      }
    }
    break;
  }
}

void Ppu::enterOamSearch() {
  mode_ = LcdMode::OamSearch;
  setStatMode(LcdMode::OamSearch);
  checkStatInterrupt();
}

void Ppu::enterPixelTransfer() {
  mode_ = LcdMode::PixelTransfer;
  setStatMode(LcdMode::PixelTransfer);
}

void Ppu::enterHBlank() {
  mode_ = LcdMode::HBlank;
  setStatMode(LcdMode::HBlank);
  checkStatInterrupt();
}

void Ppu::enterVBlank() {
  mode_ = LcdMode::VBlank;
  setStatMode(LcdMode::VBlank);
  frame_ready_ = true;

  // Request V-Blank interrupt
  uint8_t if_reg = mmu_->get(0xFF0F);
  mmu_->set(0xFF0F, if_reg | cpu::InterruptFlag::VBlank);

  checkStatInterrupt();
}

void Ppu::setStatMode(LcdMode mode) {
  uint8_t s = stat();
  s = (s & 0xFC) | static_cast<uint8_t>(mode);

  // Update LYC==LY coincidence flag (bit 2)
  bool coincidence = (ly() == lyc());
  s = (s & ~0x04) | (coincidence ? 0x04 : 0x00);

  mmu_->set(0xFF41, s);
}

void Ppu::checkStatInterrupt() {
  uint8_t s = stat();
  bool trigger = false;

  // Mode 0 H-Blank interrupt (bit 3)
  if ((s & 0x08) && mode_ == LcdMode::HBlank)
    trigger = true;
  // Mode 1 V-Blank interrupt (bit 4)
  if ((s & 0x10) && mode_ == LcdMode::VBlank)
    trigger = true;
  // Mode 2 OAM interrupt (bit 5)
  if ((s & 0x20) && mode_ == LcdMode::OamSearch)
    trigger = true;
  // LYC==LY coincidence interrupt (bit 6)
  if ((s & 0x40) && (ly() == lyc()))
    trigger = true;

  if (trigger) {
    uint8_t if_reg = mmu_->get(0xFF0F);
    mmu_->set(0xFF0F, if_reg | cpu::InterruptFlag::LcdStat);
  }
}

// ============================================================
// Rendering
// ============================================================

void Ppu::renderScanline() {
  uint8_t current_ly = ly();
  if (current_ly >= SCREEN_HEIGHT)
    return;

  // Clear bg color index tracking for this line
  bg_color_indices_.fill(0);

  if (lcdc() & LcdcFlag::BgEnable) {
    renderBackground(current_ly);
  }

  if ((lcdc() & LcdcFlag::WindowEnable) && (lcdc() & LcdcFlag::BgEnable)) {
    renderWindow(current_ly);
  }

  if (lcdc() & LcdcFlag::ObjEnable) {
    renderSprites(current_ly);
  }
}

void Ppu::renderBackground(uint8_t ly_val) {
  uint8_t scroll_y = scy();
  uint8_t scroll_x = scx();

  // Which tile map to use
  uint16_t tile_map_base = (lcdc() & LcdcFlag::BgTileMap) ? 0x9C00 : 0x9800;
  bool use_signed = !(lcdc() & LcdcFlag::TileDataArea);
  uint16_t tile_data_base = use_signed ? 0x9000 : 0x8000;

  uint8_t y = (ly_val + scroll_y) & 0xFF;
  uint8_t tile_row = y / 8;
  uint8_t pixel_y = y % 8;

  for (int screen_x = 0; screen_x < SCREEN_WIDTH; screen_x++) {
    uint8_t x = (screen_x + scroll_x) & 0xFF;
    uint8_t tile_col = x / 8;
    uint8_t pixel_x = x % 8;

    // Get tile ID from tile map
    uint16_t tile_map_addr = tile_map_base + tile_row * 32 + tile_col;
    uint8_t tile_id = mmu_->get(tile_map_addr);

    uint8_t color_idx = getTilePixel(tile_data_base, tile_id, pixel_x, pixel_y, use_signed);
    bg_color_indices_[screen_x] = color_idx;

    framebuffer_[ly_val * SCREEN_WIDTH + screen_x] = decodeColor(bgp(), color_idx);
  }
}

void Ppu::renderWindow(uint8_t ly_val) {
  uint8_t win_y = wy();
  uint8_t win_x = wx();

  // Window is visible only if WY <= LY and WX is in range
  if (ly_val < win_y || win_x > 166) {
    return;
  }

  uint16_t tile_map_base = (lcdc() & LcdcFlag::WindowTileMap) ? 0x9C00 : 0x9800;
  bool use_signed = !(lcdc() & LcdcFlag::TileDataArea);
  uint16_t tile_data_base = use_signed ? 0x9000 : 0x8000;

  uint8_t tile_row = window_line_ / 8;
  uint8_t pixel_y = window_line_ % 8;

  int wx_offset = win_x - 7; // WX is offset by 7

  bool drew_anything = false;

  for (int screen_x = 0; screen_x < SCREEN_WIDTH; screen_x++) {
    int window_x = screen_x - wx_offset;
    if (window_x < 0)
      continue;

    drew_anything = true;
    uint8_t tile_col = static_cast<uint8_t>(window_x) / 8;
    uint8_t pixel_x = static_cast<uint8_t>(window_x) % 8;

    uint16_t tile_map_addr = tile_map_base + tile_row * 32 + tile_col;
    uint8_t tile_id = mmu_->get(tile_map_addr);

    uint8_t color_idx = getTilePixel(tile_data_base, tile_id, pixel_x, pixel_y, use_signed);
    bg_color_indices_[screen_x] = color_idx;

    framebuffer_[ly_val * SCREEN_WIDTH + screen_x] = decodeColor(bgp(), color_idx);
  }

  if (drew_anything) {
    window_line_++;
  }
}

void Ppu::renderSprites(uint8_t ly_val) {
  bool tall_sprites = (lcdc() & LcdcFlag::ObjSize) != 0;
  int sprite_height = tall_sprites ? 16 : 8;

  // Collect sprites on this scanline (max 10 per line)
  struct VisibleSprite {
    SpriteAttr attr;
    uint8_t oam_index;
  };
  std::array<VisibleSprite, 10> visible;
  int visible_count = 0;

  for (int i = 0; i < 40 && visible_count < 10; i++) {
    uint16_t oam_addr = 0xFE00 + i * 4;
    SpriteAttr sprite;
    sprite.y = mmu_->get(oam_addr);
    sprite.x = mmu_->get(oam_addr + 1);
    sprite.tile = mmu_->get(oam_addr + 2);
    sprite.flags = mmu_->get(oam_addr + 3);

    int sprite_y = sprite.y - 16;
    if (ly_val >= sprite_y && ly_val < sprite_y + sprite_height) {
      visible[visible_count++] = {sprite, static_cast<uint8_t>(i)};
    }
  }

  // Draw sprites in reverse order (lower OAM index = higher priority, drawn last)
  for (int i = visible_count - 1; i >= 0; i--) {
    const auto &sprite = visible[i].attr;

    int sprite_y = sprite.y - 16;
    int sprite_x = sprite.x - 8;

    int row = ly_val - sprite_y;
    if (sprite.flipY()) {
      row = (sprite_height - 1) - row;
    }

    uint8_t tile_id = sprite.tile;
    if (tall_sprites) {
      tile_id &= 0xFE; // Top tile is even, bottom tile is odd
      if (row >= 8) {
        tile_id |= 0x01;
        row -= 8;
      }
    }

    // Sprite tiles always at 0x8000
    uint16_t tile_addr = 0x8000 + tile_id * 16 + row * 2;
    uint8_t lo = mmu_->get(tile_addr);
    uint8_t hi = mmu_->get(tile_addr + 1);

    for (int px = 0; px < 8; px++) {
      int screen_x = sprite_x + px;
      if (screen_x < 0 || screen_x >= SCREEN_WIDTH)
        continue;

      int bit = sprite.flipX() ? px : (7 - px);
      uint8_t color_idx = ((hi >> bit) & 1) << 1 | ((lo >> bit) & 1);

      // Color 0 is transparent for sprites
      if (color_idx == 0)
        continue;

      // BG priority: if sprite has priority flag and BG color is not 0, skip
      if (sprite.priority() && bg_color_indices_[screen_x] != 0)
        continue;

      uint8_t palette = sprite.palette() ? obp1() : obp0();
      framebuffer_[ly_val * SCREEN_WIDTH + screen_x] =
          decodeColor(palette, color_idx);
    }
  }
}

uint8_t Ppu::getTilePixel(uint16_t tile_data_base, uint8_t tile_id,
                          uint8_t px, uint8_t py, bool use_signed) const {
  uint16_t tile_addr;
  if (use_signed) {
    // Signed addressing: tile_id is treated as signed, base is 0x9000
    int8_t signed_id = static_cast<int8_t>(tile_id);
    tile_addr = static_cast<uint16_t>(tile_data_base + signed_id * 16);
  } else {
    tile_addr = tile_data_base + tile_id * 16;
  }

  tile_addr += py * 2;

  uint8_t lo = mmu_->get(tile_addr);
  uint8_t hi = mmu_->get(tile_addr + 1);

  int bit = 7 - px;
  return ((hi >> bit) & 1) << 1 | ((lo >> bit) & 1);
}

Color Ppu::decodeColor(uint8_t palette, uint8_t color_idx) const {
  uint8_t mapped = (palette >> (color_idx * 2)) & 0x03;
  return DMG_PALETTE[mapped];
}

} // namespace ppu
