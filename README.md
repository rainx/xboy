# XBoy

[![CI](https://github.com/rainx/xboy/actions/workflows/ci.yml/badge.svg)](https://github.com/rainx/xboy/actions/workflows/ci.yml)
[![Release](https://github.com/rainx/xboy/actions/workflows/release.yml/badge.svg)](https://github.com/rainx/xboy/actions/workflows/release.yml)
[![Deploy Web](https://github.com/rainx/xboy/actions/workflows/deploy-web.yml/badge.svg)](https://github.com/rainx/xboy/actions/workflows/deploy-web.yml)

A Game Boy (DMG) emulator written in C++17, with native SDL2 and WebAssembly builds.

**[Play in your browser](https://rainx.github.io/xboy/)** — load any `.gb` ROM file and play instantly, no install required.

## Features

- **CPU**: Full LR35902 instruction set (256 base + 256 CB-prefix opcodes)
- **MMU**: Complete 64KB address space mapping (ROM, VRAM, WRAM, OAM, I/O, HRAM)
- **PPU**: Background, window, and sprite rendering with correct LCD mode timing
- **Timer**: DIV, TIMA, TMA, TAC registers with overflow interrupt
- **Interrupts**: V-Blank, LCD STAT, Timer, Serial, Joypad — with priority dispatch
- **Input**: Keyboard-mapped joypad (arrow keys, Z/X, Enter/Backspace)
- **Display**: SDL2 rendering at 3x scale (480x432)
- **APU**: All 4 sound channels (square x2, wave, noise) with SDL2 audio output
- **Cartridge**: ROM-Only, MBC1, MBC3 (with RTC), MBC5 (with RAM + battery save)
- **Save States**: F5 to save, F7 to load (binary format with magic header)
- **Debugger**: Interactive CPU debugger with breakpoints, disassembly, memory inspection
- **Test Runner**: Headless mode for running Blargg CPU test ROMs
- **Web Version**: Full-featured browser-based emulator via WebAssembly
  - Drag-and-drop ROM loading
  - 4 save state slots
  - Adjustable display scale (1x–5x)
  - Virtual touch controls for mobile

## Controls

| Key | Function |
|-----|----------|
| Arrow keys | D-Pad |
| Z | A |
| X | B |
| Enter | Start |
| Backspace | Select |
| F5 | Save state |
| F7 | Load state |
| F12 | Toggle debugger |
| Escape | Quit |

## Build

### Native (macOS)

```bash
brew install cmake googletest sdl2
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
```

### Native (Ubuntu / Debian)

```bash
sudo apt-get install cmake libsdl2-dev libgtest-dev zlib1g-dev
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Native (Windows)

```powershell
vcpkg install sdl2:x64-windows gtest:x64-windows
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_INSTALLATION_ROOT/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

### WebAssembly

Requires [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html):

```bash
emcmake cmake -B build_web -DCMAKE_BUILD_TYPE=Release
cmake --build build_web --parallel
# Copy artifacts to web directory
cp build_web/xboy-wasm.js build_web/xboy-wasm.wasm web/js/
# Open web/index.html in a browser (use a local server for WASM)
```

Optional (for cartridge header regeneration):

```
brew install kaitai-struct-compiler
```

### Run

```
./xboy <path_to_rom.gb>
```

### Run with debugger

```
./xboy --debug <path_to_rom.gb>
```

### Run Blargg test ROM

```
./xboy --test <path_to_test_rom.gb>
```

### Test

```
cd build && ctest --output-on-failure
```

## Architecture

```
src/
├── cpu/
│   ├── registers.hpp    # A/F/B/C/D/E/H/L, SP, PC, flag accessors
│   ├── cpu.hpp/cpp      # Fetch-decode-execute loop, ALU, interrupt handling
│   ├── opcodes.cpp      # All 512 opcodes
│   └── interrupts.hpp   # Interrupt flag definitions
├── mmu/
│   ├── memory.hpp/cpp   # Abstract Memory interface (get/set/getWord/setWord)
│   ├── mmunit.hpp/cpp   # 64KB address decoder (routes to VRAM/WRAM/IO/cartridge)
│   ├── linear-memory.hpp/cpp  # Fixed-size memory template
│   └── cartridge/
│       ├── cartridge.hpp/cpp   # Base class + factory (powerUp)
│       ├── rom-only-cartridge.hpp  # 32KB ROM-only
│       ├── mbc1.hpp            # MBC1 bank switching
│       ├── mbc3.hpp            # MBC3 with RTC
│       └── mbc5.hpp            # MBC5 with rumble
├── ppu/
│   └── ppu.hpp/cpp      # LCD mode state machine + BG/Window/Sprite renderer
├── timer/
│   └── timer.hpp/cpp    # DIV/TIMA/TMA/TAC with overflow interrupt
├── apu/
│   ├── apu.hpp/cpp      # Audio processing unit + frame sequencer
│   ├── channel1.hpp/cpp # Square wave with sweep
│   ├── channel2.hpp/cpp # Square wave
│   ├── channel3.hpp/cpp # Programmable wave
│   └── channel4.hpp/cpp # Noise (LFSR)
├── input/
│   └── joypad.hpp/cpp   # JOYP register (0xFF00) + button state
├── state/
│   ├── serializable.hpp # Binary serialization helpers
│   └── save_state.hpp/cpp # Save state manager
├── debugger/
│   ├── debugger.hpp/cpp     # Interactive debugger with breakpoints
│   └── disassembler.hpp/cpp # LR35902 disassembler (all 512 opcodes)
├── test_runner/
│   └── headless_runner.hpp/cpp # Headless emulation for test ROMs
├── platform/
│   ├── sdl_display.hpp/cpp  # SDL2 window + texture rendering
│   ├── sdl_input.hpp/cpp    # Keyboard → Joypad mapping
│   ├── sdl_audio.hpp/cpp    # SDL2 audio output
│   ├── web_display.hpp/cpp  # HTML5 Canvas rendering (Emscripten)
│   ├── web_input.hpp/cpp    # Browser keyboard/gamepad input
│   ├── web_audio.hpp/cpp    # Web Audio API output
│   └── web_filesystem.hpp/cpp # Emscripten virtual filesystem
├── main.cpp             # Native emulation loop + CLI
└── web_main.cpp         # WebAssembly entry point

web/
├── index.html           # Emulator UI (drag-and-drop ROM loading)
├── css/xboy.css         # Apple-inspired minimalist styling
└── js/
    ├── xboy.js          # Main emulator JavaScript logic
    ├── rom-loader.js    # ROM file loading
    ├── save-states.js   # Save state management
    └── controls.js      # Input/keyboard handling
```

## Emulation Loop

```
while (running) {
    cycles = cpu.step()
    timer.step(cycles)
    ppu.step(cycles)
    cpu.handleInterrupts()
    if (frame ready) render + poll input
}
```

## Debugger

Start with `--debug` flag or press F12 during emulation:

```
(xdb) h
Commands:
  s/step          Step one instruction
  n/next          Step over CALL
  c/continue      Run until breakpoint
  b/break <addr>  Set breakpoint (hex address)
  d/delete <id>   Remove breakpoint/watchpoint
  l/list          List breakpoints/watchpoints
  r/regs          Show CPU registers
  m/mem <addr> [n] Hex dump n bytes
  dis [addr] [n]  Disassemble n instructions
  w/watch <addr>  Watch memory address for changes
  q/quit          Exit debugger
```

## Roadmap

- [x] CPU — full instruction set with cycle-accurate timing
- [x] MMU — complete address space with echo RAM, I/O, serial capture
- [x] PPU — background, window, sprite rendering
- [x] Timer — DIV/TIMA with interrupt
- [x] Input — keyboard-mapped joypad
- [x] Display — SDL2 output
- [x] MBC1 cartridge support
- [x] MBC3 / MBC5 cartridge support
- [x] APU (audio) — all 4 channels with SDL2 output
- [x] Blargg CPU test ROM verification
- [x] Save states
- [x] Debugger
- [x] WebAssembly — browser-based emulator with full feature parity
- [x] CI/CD — automated builds, testing, and deployment
- [ ] Game Boy Color (CGB) support
- [ ] Game Link Cable (serial) emulation
