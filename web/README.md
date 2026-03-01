# XBoy WebAssembly Emulator

A Game Boy emulator compiled to WebAssembly for in-browser gameplay.

## Features

- **Full Game Boy Compatibility**: Supports standard Game Boy (.gb) and Game Boy Color (.gbc) ROMs
- **WebAssembly Performance**: Near-native performance in modern browsers
- **Save States**: Browser-based save state management with 4 slots
- **Responsive Design**: Works on desktop and mobile devices
- **Virtual Controls**: Touch-friendly controls for mobile devices
- **Keyboard Support**: Full keyboard mapping for desktop gaming
- **Drag & Drop**: Easy ROM loading with drag-and-drop interface
- **Adjustable Scaling**: Multiple display scale options (1x-5x)

## Controls

### Keyboard
- **Arrow Keys**: D-Pad (Up/Down/Left/Right)
- **Z**: A button
- **X**: B button  
- **Enter**: Start button
- **Backspace**: Select button

### Mobile Controls
- **Virtual D-Pad**: Touch-based directional control
- **Action Buttons**: A, B, Start, Select buttons

### Gamepad Support
- Game controllers are automatically detected and mapped to Game Boy controls

## Quick Start

1. **Open the Emulator**: Load `index.html` in a modern web browser
2. **Load a ROM**: 
   - Click "Load ROM" and select a file, OR
   - Drag and drop a ROM file onto the drop zone
3. **Start Playing**: Click "Start" to begin emulation
4. **Save Progress**: Use save state slots to save your progress

## ROM Loading

The emulator supports:
- **Drag & Drop**: Drag ROM files directly onto the browser window
- **File Picker**: Click "Load ROM" to browse for ROM files
- **File Types**: `.gb` and `.gbc` files up to 8MB

## Save States

- **4 Save Slots**: Each ROM can have 4 independent save states
- **Browser Storage**: Save states are stored in your browser (IndexedDB/localStorage)
- **Screenshots**: Save states include screenshot previews
- **Cross-Session**: Save states persist between browser sessions

## Performance Optimization

- **Adaptive Quality**: Automatically adjusts performance based on device capabilities
- **Frame Skipping**: Optimized rendering for smooth gameplay
- **Memory Management**: Efficient WebAssembly memory usage
- **Audio Buffering**: Smooth audio with minimal latency

## Browser Compatibility

- **Chrome**: Full support (recommended)
- **Firefox**: Full support
- **Safari**: Full support (requires WebAssembly enabled)
- **Edge**: Full support
- **Mobile iOS**: Supported with touch controls
- **Mobile Android**: Supported with touch controls

## Technical Details

### Architecture
- **Core Emulation**: C++17 codebase compiled to WebAssembly
- **Web Interface**: HTML5, CSS3, JavaScript ES6+
- **Audio**: Web Audio API for 4-channel Game Boy sound
- **Graphics**: HTML5 Canvas with pixel-perfect rendering
- **Storage**: IndexedDB and localStorage for save states

### Performance
- **Target FPS**: 60 FPS
- **Audio**: 44.1kHz stereo output
- **Resolution**: 160x144 Game Boy native resolution
- **Scaling**: 1x-5x with nearest-neighbor filtering

## Troubleshooting

### Common Issues

**Emulator doesn't load:**
- Ensure your browser supports WebAssembly
- Check browser console for error messages
- Try refreshing the page

**ROM won't load:**
- Verify file is a valid `.gb` or `.gbc` ROM
- Check file size (maximum 8MB)
- Try a different ROM to test functionality

**No sound:**
- Click anywhere on the page to activate audio context
- Check browser audio permissions
- Ensure volume is up

**Poor performance:**
- Reduce display scale in settings
- Close other browser tabs
- Try a different browser (Chrome recommended)

### Developer Console

Open the browser developer console (F12) to see:
- Loading progress messages
- ROM loading errors
- Performance statistics
- Debug information

## Sample ROMs

The `web/assets/` directory contains sample ROMs for testing:
- `tetris-jue-v1.1.gb` - Tetris (public domain)

## Development

### Building from Source

```bash
# Clone repository
git clone <repository-url>
cd xboy

# Install Emscripten
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

# Build WebAssembly version
./build_web.sh

# Build native version
./build_native.sh
```

### File Structure

```
web/
├── index.html          # Main emulator interface
├── js/
│   ├── xboy.js         # Main emulator logic
│   ├── rom-loader.js    # ROM loading functionality
│   ├── save-states.js   # Save state management
│   └── controls.js      # Input handling
├── css/
│   └── xboy.css        # Styling and responsive design
└── assets/             # Sample ROMs and resources
```

## License

This project maintains the same license as the original xboy emulator.

## Contributing

Contributions are welcome! Please see the main repository for contribution guidelines.