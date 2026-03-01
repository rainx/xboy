# XBoy WebAssembly Support - Implementation Plan

Based on comprehensive analysis of the xboy codebase and WebAssembly requirements, here's a detailed implementation plan:

## Overview

XBoy is exceptionally well-architected for WebAssembly porting, with clean separation between core emulation logic and platform-specific SDL2 code. The plan below will add web support while preserving native builds.

## Phase 1: Build System & Platform Abstraction (High Priority)

### 1.1 CMakeLists.txt Modifications
- Add Emscripten toolchain detection and configuration
- Create conditional compilation for native vs web builds
- Set up Embind for C++/JavaScript interop
- Configure linker flags for WebAssembly output

### 1.2 Web Platform Layer
Create `src/platform/web_*.cpp` files to replace SDL2 components:
- `web_display.cpp` - HTML5 Canvas rendering using Emscripten's SDL2 support
- `web_audio.cpp` - Web Audio API integration 
- `web_input.cpp` - Keyboard/mouse event handling
- `web_filesystem.cpp` - Fetch API for ROM loading and IndexedDB for save states

### 1.3 C++/JavaScript Interop
Implement Embind bindings in `bindings.cpp`:
- `WebEmulator` class exposing core functionality to JavaScript
- Methods for frame stepping, button input, save/load states
- Audio callback integration with Web Audio API

## Phase 2: Core Web Features (Medium Priority)

### 2.1 ROM Loading System
- HTML5 file input and drag-and-drop interface
- Binary file processing and validation
- Integration with cartridge loading system

### 2.2 Save State Management
- Browser storage abstraction (IndexedDB for large save states)
- Compression for storage efficiency
- UI for managing multiple save slots

### 2.3 Display Rendering
- HTML5 Canvas integration with pixel-perfect scaling
- Framebuffer conversion from C++ Color struct to JavaScript ImageData
- Performance optimizations (frame skipping, dirty rectangles)

### 2.4 Audio System
- Web Audio API implementation for all 4 Game Boy audio channels
- Sample rate conversion and buffering
- Audio context management for browser compatibility

## Phase 3: User Interface & Interaction (Medium Priority)

### 3.1 Debugger Interface
- Web-based version of the CLI debugger
- Real-time register display
- Memory viewer with hex dump
- Breakpoint and watchpoint management
- Disassembler view

### 3.2 Responsive Design
- Mobile-optimized layout with touch controls
- Desktop keyboard mapping
- Virtual gamepad for touch devices
- Responsive controls panel

### 3.3 Performance Optimization
- Adaptive quality settings based on device capabilities
- Frame skipping for low-end devices
- Memory management optimizations

## Phase 4: Polish & Deployment (Low Priority)

### 4.1 Build Scripts
- Automated build scripts for native and web builds
- Optimization flags for different build types
- Docker container for reproducible builds

### 4.2 Demo & Documentation
- Demo page with sample ROMs
- Comprehensive documentation for web features
- Performance benchmarking tools

### 4.3 Advanced Features
- Web Workers for heavy computations
- Progressive Web App (PWA) support
- Game controller API support

## Technical Implementation Details

### File Structure Additions
```
src/platform/
├── web_display.hpp/cpp
├── web_audio.hpp/cpp  
├── web_input.hpp/cpp
└── web_filesystem.hpp/cpp

bindings.cpp              # Embind bindings

web/
├── index.html            # Main interface
├── js/
│   ├── xboy.js           # Main JavaScript logic
│   ├── debugger.js       # Debugger UI
│   ├── audio.js          # Web Audio wrapper
│   └── ui.js             # UI management
├── css/
│   └── xboy.css          # Styling
└── assets/               # Sample ROMs, icons
```

### Build Commands
```bash
# Native build (existing)
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Web build (new)
source /path/to/emsdk/emsdk_env.sh
mkdir build_web && cd build_web
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release
emmake make
```

### Key Implementation Considerations

1. **Memory Management**: The existing code uses appropriate data structures for WASM compilation
2. **Audio Threading**: Web Audio API requires different approach than SDL2 threading
3. **File System**: Virtual file system abstraction needed for web environment
4. **Debugger UI**: Complete rewrite from CLI to web-based interface
5. **Performance**: Frame timing and synchronization will need browser-specific implementation

## Phase 1 Detailed Implementation Plan

### 1.1 CMakeLists.txt Modifications

#### Changes to Root CMakeLists.txt
```cmake
# Add after project definition
if(DEFINED ENV{EM_CONFIG})
    set(EMSCRIPTEN TRUE)
    message(STATUS "Emscripten detected, configuring for WebAssembly build")
endif()

# Platform-specific configuration
if(EMSCRIPTEN)
    # Emscripten settings
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_SDL=2")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s ALLOW_MEMORY_GROWTH=1")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s EXPORTED_FUNCTIONS=_main,_malloc,_free")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s EXPORTED_RUNTIME_METHODS=ccall,cwrap")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s MODULARIZE=1")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s EXPORT_NAME='XBoy'")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s WASM=1")
    
    # Define for conditional compilation
    add_definitions(-DEMSCRIPTEN)
else()
    # Native configuration (existing)
    include(CTest)
    include(GoogleTest)
    find_package(GTest REQUIRED)
    find_package(SDL2 REQUIRED)
    enable_testing()
endif()

# Platform-specific sources
if(EMSCRIPTEN)
    set(PLATFORM_SOURCES
        src/platform/web_display.cpp
        src/platform/web_audio.cpp
        src/platform/web_input.cpp
        src/platform/web_filesystem.cpp
        bindings.cpp
    )
else()
    set(PLATFORM_SOURCES
        src/platform/sdl_display.cpp
        src/platform/sdl_audio.cpp
        src/platform/sdl_input.cpp
    )
endif()

# Conditional linking
if(NOT EMSCRIPTEN)
    target_include_directories(xboy PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(xboy PRIVATE ${SDL2_LIBRARIES})
endif()
```

### 1.2 Web Platform Layer Implementation

#### web_display.hpp
```cpp
#pragma once
#include "ppu/ppu.hpp"
#include <cstdint>

namespace platform {

class WebDisplay {
public:
    WebDisplay(int scale = 3);
    ~WebDisplay();
    
    void render(const std::array<ppu::Color, ppu::SCREEN_WIDTH * ppu::SCREEN_HEIGHT> &fb);
    
private:
    int scale_;
    uint32_t canvas_id_;
};

} // namespace platform
```

#### web_display.cpp
```cpp
#include "platform/web_display.hpp"
#include <emscripten.h>
#include <SDL.h>

namespace platform {

WebDisplay::WebDisplay(int scale) : scale_(scale) {
    // Initialize SDL2 for Emscripten (provides Canvas context)
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }
    
    // Create window (Emscripten maps this to Canvas)
    SDL_Window *window = SDL_CreateWindow("XBoy",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          ppu::SCREEN_WIDTH * scale_, ppu::SCREEN_HEIGHT * scale_,
                                          SDL_WINDOW_SHOWN);
    
    if (!window) {
        throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
    }
    
    // Store canvas ID for JavaScript interop
    canvas_id_ = reinterpret_cast<uint32_t>(window);
}

WebDisplay::~WebDisplay() {
    SDL_Quit();
}

void WebDisplay::render(const std::array<ppu::Color, ppu::SCREEN_WIDTH * ppu::SCREEN_HEIGHT> &fb) {
    // Emscripten SDL2 handles Canvas rendering
    // This will be called from the main emulation loop
}

} // namespace platform
```

#### web_audio.hpp
```cpp
#pragma once
#include <cstdint>

namespace platform {

class WebAudio {
public:
    WebAudio();
    ~WebAudio();
    
    void pushSample(float left, float right);
    uint32_t getQueuedBytes() const;
    
private:
    uint32_t audio_context_id_;
};

} // namespace platform
```

#### web_input.hpp
```cpp
#pragma once
#include "input/joypad.hpp"
#include <memory>

namespace platform {

class WebInput {
public:
    explicit WebInput(std::shared_ptr<input::Joypad> joypad);
    ~WebInput();
    
    bool poll() const;
    
private:
    std::shared_ptr<input::Joypad> joypad_;
};

} // namespace platform
```

#### web_filesystem.hpp
```cpp
#pragma once
#include <string>
#include <vector>

namespace platform {

class WebFileSystem {
public:
    static std::vector<uint8_t> readFile(const std::string& path);
    static bool writeFile(const std::string& path, const std::vector<uint8_t>& data);
    static bool fileExists(const std::string& path);
};

} // namespace platform
```

### 1.3 C++/JavaScript Interop (bindings.cpp)

```cpp
#include <emscripten/bind.h>
#include "cpu/cpu.hpp"
#include "mmu/mmunit.hpp"
#include "ppu/ppu.hpp"
#include "apu/apu.hpp"
#include "input/joypad.hpp"
#include "timer/timer.hpp"
#include "state/save_state.hpp"
#include "platform/web_display.hpp"
#include "platform/web_audio.hpp"
#include "platform/web_input.hpp"
#include "platform/web_filesystem.hpp"

class WebEmulator {
public:
    WebEmulator(const std::string& romData) {
        // Create temporary file for ROM data
        tempRomPath_ = "/tmp/temp_rom.gb";
        std::vector<uint8_t> romBytes(romData.begin(), romData.end());
        platform::WebFileSystem::writeFile(tempRomPath_, romBytes);
        
        // Initialize emulator components
        mmu_ = mmu::Mmunit::powerUp(tempRomPath_);
        cpu_ = std::make_unique<cpu::Cpu>(mmu_);
        timer_ = std::make_unique<timer::Timer>(mmu_);
        ppu_ = std::make_unique<ppu::Ppu>(mmu_);
        apu_ = std::make_unique<apu::Apu>(mmu_);
        joypad_ = std::make_shared<input::Joypad>(mmu_);
        
        // Initialize platform components
        display_ = std::make_unique<platform::WebDisplay>(3);
        audio_ = std::make_unique<platform::WebAudio>();
        input_ = std::make_unique<platform::WebInput>(joypad_);
        
        setupAudioCallback();
    }
    
    ~WebEmulator() {
        // Clean up temporary ROM file
        platform::WebFileSystem::writeFile(tempRomPath_, {});
    }
    
    void stepFrame() {
        uint32_t frame_cycles = 0;
        while (frame_cycles < 70224) { // CYCLES_PER_FRAME
            uint8_t cycles = cpu_->step();
            timer_->step(cycles);
            ppu_->step(cycles);
            apu_->step(cycles);
            frame_cycles += cycles;
            
            uint8_t int_cycles = cpu_->handleInterrupts();
            frame_cycles += int_cycles;
        }
        
        // Render frame if ready
        if (ppu_->frameReady()) {
            display_->render(ppu_->getFrameBuffer());
            ppu_->clearFrameReady();
        }
    }
    
    void setButtonState(int button, bool pressed) {
        joypad_->setButtonState(static_cast<input::JoypadButton>(button), pressed);
    }
    
    void saveState(int slot = 1) {
        state::SaveStateManager saveMgr(*cpu_, mmu_, *ppu_, *timer_, *apu_, joypad_, "");
        saveMgr.save(slot);
    }
    
    bool loadState(int slot = 1) {
        state::SaveStateManager saveMgr(*cpu_, mmu_, *ppu_, *timer_, *apu_, joypad_, "");
        return saveMgr.load(slot);
    }
    
private:
    std::shared_ptr<mmu::Mmunit> mmu_;
    std::unique_ptr<cpu::Cpu> cpu_;
    std::unique_ptr<timer::Timer> timer_;
    std::unique_ptr<ppu::Ppu> ppu_;
    std::unique_ptr<apu::Apu> apu_;
    std::shared_ptr<input::Joypad> joypad_;
    
    std::unique_ptr<platform::WebDisplay> display_;
    std::unique_ptr<platform::WebAudio> audio_;
    std::unique_ptr<platform::WebInput> input_;
    
    std::string tempRomPath_;
    
    void setupAudioCallback() {
        apu_->setSampleCallback([this](float left, float right) {
            audio_->pushSample(left, right);
        });
    }
};

// Embind bindings
EMSCRIPTEN_BINDINGS(xboy) {
    emscripten::class_<WebEmulator>("WebEmulator")
        .constructor<std::string>()
        .function("stepFrame", &WebEmulator::stepFrame)
        .function("setButtonState", &WebEmulator::setButtonState)
        .function("saveState", &WebEmulator::saveState)
        .function("loadState", &WebEmulator::loadState);
}
```

### 1.4 Build Scripts

#### build_web.sh
```bash
#!/bin/bash
set -e

# Set up Emscripten environment
source /path/to/emsdk/emsdk_env.sh

# Clean previous build
rm -rf build_web
mkdir build_web
cd build_web

# Configure for web
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
emmake make -j$(nproc)

echo "WebAssembly build complete!"
echo "Open build_web/xboy.html in your browser"
```

#### build_native.sh
```bash
#!/bin/bash
set -e

# Clean previous build
rm -rf build
mkdir build
cd build

# Configure for native
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
make -j$(nproc)

echo "Native build complete!"
echo "Run ./build/xboy <rom_path.gb>"
```

### 1.5 File Structure After Phase 1

```
xboy/
├── CMakeLists.txt (modified)
├── build_web.sh (new)
├── build_native.sh (new)
├── src/
│   ├── cpu/, mmu/, ppu/, etc. (unchanged)
│   ├── platform/
│   │   ├── sdl_*.cpp (existing)
│   │   ├── web_*.cpp (new)
│   │   └── web_*.hpp (new)
│   └── main.cpp (unchanged)
├── bindings.cpp (new)
├── build/
└── build_web/
    ├── xboy.html (generated)
    ├── xboy.js (generated)
    ├── xboy.wasm (generated)
    └── xboy.data (generated)
```

## Implementation Order for Phase 1

1. **Modify CMakeLists.txt** - Add Emscripten support and conditional compilation
2. **Create web platform headers** - Define interfaces for web_*.hpp files
3. **Implement basic web platform stubs** - Create minimal implementations for web_*.cpp
4. **Create bindings.cpp** - Implement Embind bindings for WebEmulator class
5. **Test compilation** - Verify both native and web builds work
6. **Refine implementations** - Add proper implementations to web platform classes

## Testing Strategy for Phase 1

1. **Build verification**: Ensure both native and web targets compile successfully
2. **Basic functionality**: Test ROM loading and initialization in web build
3. **Platform abstraction**: Verify SDL2 and web platforms produce equivalent behavior
4. **Performance profiling**: Check that web build performance is acceptable

This phase establishes the foundation for all subsequent web features while maintaining compatibility with the native SDL2 build.