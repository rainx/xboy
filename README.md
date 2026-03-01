# XBoy

A Game Boy (DMG) emulator written in C++17.

## Features

- **CPU**: Full LR35902 instruction set (256 base + 256 CB-prefix opcodes)
- **MMU**: Complete 64KB address space mapping (ROM, VRAM, WRAM, OAM, I/O, HRAM)
- **PPU**: Background, window, and sprite rendering with correct LCD mode timing
- **Timer**: DIV, TIMA, TMA, TAC registers with overflow interrupt
- **Interrupts**: V-Blank, LCD STAT, Timer, Serial, Joypad — with priority dispatch
- **Input**: Keyboard-mapped joypad (arrow keys, Z/X, Enter/Backspace)
- **Display**: SDL2 rendering at 3x scale (480x432)
- **Cartridge**: ROM-Only and MBC1 (with RAM + battery save)

## Controls

| Key | Game Boy |
|-----|----------|
| Arrow keys | D-Pad |
| Z | A |
| X | B |
| Enter | Start |
| Backspace | Select |
| Escape | Quit |

## Build

### Prerequisites (macOS)

```
brew install cmake googletest sdl2
```

Optional (for cartridge header regeneration):

```
brew install kaitai-struct-compiler
```

### Compile

```
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
```

### Run

```
./xboy <path_to_rom.gb>
```

### Test

```
./xboy_test
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
│       └── mbc1.hpp            # MBC1 bank switching
├── ppu/
│   └── ppu.hpp/cpp      # LCD mode state machine + BG/Window/Sprite renderer
├── timer/
│   └── timer.hpp/cpp    # DIV/TIMA/TMA/TAC with overflow interrupt
├── input/
│   └── joypad.hpp/cpp   # JOYP register (0xFF00) + button state
├── platform/
│   ├── sdl_display.hpp/cpp  # SDL2 window + texture rendering
│   └── sdl_input.hpp/cpp    # Keyboard → Joypad mapping
└── main.cpp             # Emulation loop
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

## Roadmap

- [x] CPU — full instruction set with cycle-accurate timing
- [x] MMU — complete address space with echo RAM, I/O, serial capture
- [x] PPU — background, window, sprite rendering
- [x] Timer — DIV/TIMA with interrupt
- [x] Input — keyboard-mapped joypad
- [x] Display — SDL2 output
- [x] MBC1 cartridge support
- [ ] Blargg CPU test ROM verification
- [ ] MBC3 / MBC5 cartridge support
- [ ] Save states
- [ ] APU (audio)
- [ ] Debugger
