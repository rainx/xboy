# Phase 1: Build System & Platform Abstraction Implementation

## Detailed Implementation Steps

### 1. CMakeLists.txt Modifications

#### New CMakeLists.txt Structure
```cmake
cmake_minimum_required(VERSION 3.10)
project(xboy VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Detect Emscripten
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
    # Native configuration
    include(CTest)
    include(GoogleTest)
    find_package(GTest REQUIRED)
    find_package(SDL2 REQUIRED)
    enable_testing()
endif()

# Core emulator sources (no SDL / platform code)
file(GLOB CORE_SOURCES
    src/mmu/*.cpp
    src/mmu/cartridge/*.cpp
    src/cpu/*.cpp
    src/timer/*.cpp
    src/ppu/*.cpp
    src/input/*.cpp
    src/apu/*.cpp
    src/test_runner/*.cpp
    src/state/*.cpp
    src/debugger/*.cpp
    src/kaitai-struct-gen/*.cpp
)

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

# Main executable
add_executable(xboy src/main.cpp)
target_sources(xboy PRIVATE ${CORE_SOURCES} ${PLATFORM_SOURCES})
target_include_directories(xboy PRIVATE ./src)

# Platform-specific linking
if(NOT EMSCRIPTEN)
    target_include_directories(xboy PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(xboy PRIVATE ${SDL2_LIBRARIES})
endif()

# Thirdparty dependencies
add_subdirectory(thirdparty/kaitai_struct_cpp_stl_runtime)
target_link_libraries(xboy PRIVATE kaitai_struct_cpp_stl_runtime)
target_include_directories(xboy PRIVATE ./thirdparty/kaitai_struct_cpp_stl_runtime)

# Tests (only for native builds)
if(NOT EMSCRIPTEN)
    file(GLOB TEST_SOURCES
        test/mmu/*.test.cpp
        test/cpu/*.test.cpp
        test/kaitai-struct-gen/*.test.cpp
    )

    add_executable(xboy_test test/main.cpp)
    target_sources(xboy_test PRIVATE ${CORE_SOURCES} ${TEST_SOURCES})
    target_include_directories(xboy_test PRIVATE ./src)
    target_include_directories(xboy_test PRIVATE ${GTEST_INCLUDE_DIR})
    target_link_libraries(xboy_test PRIVATE ${GTEST_LIBRARIES})
    target_link_libraries(xboy_test PRIVATE kaitai_struct_cpp_stl_runtime)
    target_include_directories(xboy_test PRIVATE ./thirdparty/kaitai_struct_cpp_stl_runtime)

    gtest_discover_tests(xboy_test)
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
```

### 2. Web Platform Layer Files

#### src/platform/web_display.hpp
```cpp
#pragma once

#include "ppu/ppu.hpp"
#include <cstdint>

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
```

#### src/platform/web_display.cpp
```cpp
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
```

#### src/platform/web_audio.hpp
```cpp
#pragma once

#include <cstdint>

namespace platform {

class WebAudio {
public:
    WebAudio();
    ~WebAudio();

    WebAudio(const WebAudio &) = delete;
    WebAudio &operator=(const WebAudio &) = delete;

    void pushSample(float left, float right);
    uint32_t getQueuedBytes() const;

    static constexpr int SAMPLE_RATE = 44100;

private:
    SDL_AudioDeviceID audio_device_;
    SDL_AudioSpec audio_spec_;
};

} // namespace platform
```

#### src/platform/web_audio.cpp
```cpp
#include "platform/web_audio.hpp"
#include <stdexcept>
#include <SDL.h>
#include <vector>
#include <mutex>

namespace platform {

WebAudio::WebAudio() {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        throw std::runtime_error(std::string("SDL_Init AUDIO failed: ") + SDL_GetError());
    }

    SDL_AudioSpec desired_spec;
    SDL_zero(desired_spec);
    desired_spec.freq = SAMPLE_RATE;
    desired_spec.format = AUDIO_F32;
    desired_spec.channels = 2;
    desired_spec.samples = 1024;
    desired_spec.callback = nullptr; // We'll use SDL_QueueAudio

    audio_device_ = SDL_OpenAudioDevice(nullptr, 0, &desired_spec, &audio_spec_, 0);
    if (audio_device_ == 0) {
        throw std::runtime_error(std::string("SDL_OpenAudioDevice failed: ") + SDL_GetError());
    }

    // Start audio playback
    SDL_PauseAudioDevice(audio_device_, 0);
}

WebAudio::~WebAudio() {
    if (audio_device_) {
        SDL_CloseAudioDevice(audio_device_);
    }
}

void WebAudio::pushSample(float left, float right) {
    float samples[2] = {left, right};
    SDL_QueueAudio(audio_device_, samples, sizeof(samples));
}

uint32_t WebAudio::getQueuedBytes() const {
    return SDL_GetQueuedAudioSize(audio_device_);
}

} // namespace platform
```

#### src/platform/web_input.hpp
```cpp
#pragma once

#include "input/joypad.hpp"
#include <memory>

namespace platform {

class WebInput {
public:
    explicit WebInput(std::shared_ptr<input::Joypad> joypad);
    ~WebInput();

    WebInput(const WebInput &) = delete;
    WebInput &operator=(const WebInput &) = delete;

    bool poll();

private:
    std::shared_ptr<input::Joypad> joypad_;
};

} // namespace platform
```

#### src/platform/web_input.cpp
```cpp
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
```

#### src/platform/web_filesystem.hpp
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

#### src/platform/web_filesystem.cpp
```cpp
#include "platform/web_filesystem.hpp"
#include <fstream>
#include <sys/stat.h>
#include <emscripten.h>

namespace platform {

std::vector<uint8_t> WebFileSystem::readFile(const std::string& path) {
    // Check if file exists
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0) {
        throw std::runtime_error("File not found: " + path);
    }

    // Read file
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + path);
    }

    return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
}

bool WebFileSystem::writeFile(const std::string& path, const std::vector<uint8_t>& data) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return file.good();
}

bool WebFileSystem::fileExists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

} // namespace platform
```

### 3. C++/JavaScript Interop (bindings.cpp)

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
#include <memory>
#include <string>

class WebEmulator {
public:
    WebEmulator(const std::string& romData) {
        // Create temporary file for ROM data
        tempRomPath_ = "/tmp/temp_rom.gb";
        std::vector<uint8_t> romBytes(romData.begin(), romData.end());
        if (!platform::WebFileSystem::writeFile(tempRomPath_, romBytes)) {
            throw std::runtime_error("Failed to write ROM to temporary file");
        }
        
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
            
            // Update joypad register (in case game wrote to FF00 select bits)
            joypad_->update();
        }
        
        // Render frame if ready
        if (ppu_->frameReady()) {
            display_->render(ppu_->getFrameBuffer());
            ppu_->clearFrameReady();
        }
        
        // Poll input
        if (!input_->poll()) {
            // Handle quit if needed
        }
        
        // Audio-based throttling
        while (audio_->getQueuedBytes() > platform::WebAudio::SAMPLE_RATE * 4 * 2 / 15) {
            emscripten_sleep(1);
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

### 4. Build Scripts

#### build_web.sh
```bash
#!/bin/bash
set -e

# Set up Emscripten environment
if [ -z "$EMSDK" ]; then
    echo "Error: EMSDK environment variable not set"
    echo "Please run 'source /path/to/emsdk/emsdk_env.sh' first"
    exit 1
fi

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

## Implementation Order

1. **Backup original CMakeLists.txt**
2. **Replace CMakeLists.txt** with the new version
3. **Create web platform header files** (web_*.hpp)
4. **Create web platform implementation files** (web_*.cpp)
5. **Create bindings.cpp** with Embind bindings
6. **Create build scripts** (build_web.sh, build_native.sh)
7. **Test native build** to ensure no regression
8. **Test web build** (requires Emscripten setup)

## Testing Phase 1

1. **Native Build Verification**:
   ```bash
   chmod +x build_native.sh
   ./build_native.sh
   ```

2. **Web Build Verification** (requires Emscripten):
   ```bash
   # First set up Emscripten
   source /path/to/emsdk/emsdk_env.sh
   
   chmod +x build_web.sh
   ./build_web.sh
   ```

3. **Basic Web Test** (JavaScript console):
   ```javascript
   // Load the xboy.js module and test basic instantiation
   XBoy().then(module => {
       const emulator = new module.WebEmulator(romData);
       emulator.stepFrame();
   });
   ```

## Expected File Structure After Phase 1

```
xboy/
├── CMakeLists.txt (modified)
├── build_web.sh (new)
├── build_native.sh (new)
├── WEBASM_PLAN.md (created)
├── PHASE1_IMPLEMENTATION.md (this file)
├── src/
│   ├── cpu/, mmu/, ppu/, etc. (unchanged)
│   ├── platform/
│   │   ├── sdl_*.cpp (existing)
│   │   ├── sdl_*.hpp (existing)
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

This completes Phase 1 - establishing the build system and platform abstraction layer. The project can now be compiled for both native SDL2 and WebAssembly targets.