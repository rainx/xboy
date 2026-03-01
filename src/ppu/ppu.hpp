#pragma once

#include "mmu/mmunit.hpp"
#include <array>
#include <cstdint>
#include <memory>

namespace ppu {

// Game Boy screen dimensions
constexpr int SCREEN_WIDTH = 160;
constexpr int SCREEN_HEIGHT = 144;
constexpr int TOTAL_SCANLINES = 154; // 144 visible + 10 V-Blank

// LCD mode durations (in T-cycles)
constexpr int MODE2_CYCLES = 80;   // OAM Search
constexpr int MODE3_CYCLES = 172;  // Pixel Transfer
constexpr int MODE0_CYCLES = 204;  // H-Blank
constexpr int SCANLINE_CYCLES = MODE2_CYCLES + MODE3_CYCLES + MODE0_CYCLES; // 456

// LCD modes
enum class LcdMode : uint8_t {
  HBlank = 0,
  VBlank = 1,
  OamSearch = 2,
  PixelTransfer = 3,
};

// LCDC register bits (0xFF40)
enum LcdcFlag : uint8_t {
  BgEnable = 0x01,        // Bit 0: BG & Window enable
  ObjEnable = 0x02,       // Bit 1: OBJ (Sprite) enable
  ObjSize = 0x04,         // Bit 2: OBJ size (0=8x8, 1=8x16)
  BgTileMap = 0x08,       // Bit 3: BG Tile Map area (0=9800, 1=9C00)
  TileDataArea = 0x10,    // Bit 4: BG & Window Tile Data (0=8800, 1=8000)
  WindowEnable = 0x20,    // Bit 5: Window enable
  WindowTileMap = 0x40,   // Bit 6: Window Tile Map area (0=9800, 1=9C00)
  LcdEnable = 0x80,       // Bit 7: LCD enable
};

// Sprite attribute (OAM entry)
struct SpriteAttr {
  uint8_t y;
  uint8_t x;
  uint8_t tile;
  uint8_t flags;

  bool priority() const { return flags & 0x80; }  // Behind BG
  bool flipY() const { return flags & 0x40; }
  bool flipX() const { return flags & 0x20; }
  bool palette() const { return flags & 0x10; }    // 0=OBP0, 1=OBP1
};

// Color value (2-bit Game Boy color: 0=lightest, 3=darkest)
// Mapped to RGBA for SDL output
struct Color {
  uint8_t r, g, b, a;
};

class Ppu {
public:
  Ppu(std::shared_ptr<mmu::Mmunit> mmu);

  // Advance PPU by the given number of T-cycles
  void step(uint8_t cycles);

  // Check if a new frame is ready for display
  bool frameReady() const { return frame_ready_; }
  void clearFrameReady() { frame_ready_ = false; }

  // Get the framebuffer (RGBA, 160x144)
  const std::array<Color, SCREEN_WIDTH * SCREEN_HEIGHT> &getFrameBuffer() const {
    return framebuffer_;
  }

private:
  std::shared_ptr<mmu::Mmunit> mmu_;

  // State
  uint16_t cycle_counter_ = 0;
  LcdMode mode_ = LcdMode::OamSearch;
  bool frame_ready_ = false;

  // Internal window line counter
  uint8_t window_line_ = 0;
  bool window_was_active_ = false;

  // Framebuffer
  std::array<Color, SCREEN_WIDTH * SCREEN_HEIGHT> framebuffer_{};

  // Per-scanline background priority (for sprite behind-BG logic)
  std::array<uint8_t, SCREEN_WIDTH> bg_color_indices_{};

  // Mode transition handlers
  void enterOamSearch();
  void enterPixelTransfer();
  void enterHBlank();
  void enterVBlank();

  // Rendering
  void renderScanline();
  void renderBackground(uint8_t ly);
  void renderWindow(uint8_t ly);
  void renderSprites(uint8_t ly);

  // Tile data helpers
  uint8_t getTilePixel(uint16_t tile_data_base, uint8_t tile_id,
                       uint8_t px, uint8_t py, bool use_signed) const;

  // Palette decoding: 2-bit index → Color
  Color decodeColor(uint8_t palette, uint8_t color_idx) const;

  // STAT interrupt check
  void checkStatInterrupt();

  // Register helpers
  uint8_t lcdc() const { return mmu_->get(0xFF40); }
  uint8_t stat() const { return mmu_->get(0xFF41); }
  uint8_t scy() const { return mmu_->get(0xFF42); }
  uint8_t scx() const { return mmu_->get(0xFF43); }
  uint8_t ly() const { return mmu_->get(0xFF44); }
  uint8_t lyc() const { return mmu_->get(0xFF45); }
  uint8_t bgp() const { return mmu_->get(0xFF47); }
  uint8_t obp0() const { return mmu_->get(0xFF48); }
  uint8_t obp1() const { return mmu_->get(0xFF49); }
  uint8_t wy() const { return mmu_->get(0xFF4A); }
  uint8_t wx() const { return mmu_->get(0xFF4B); }

  void setLY(uint8_t val) { mmu_->set(0xFF44, val); }
  void setStatMode(LcdMode mode);
};

} // namespace ppu
