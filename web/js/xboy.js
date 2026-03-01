// XBoy WebAssembly Emulator - Main JavaScript Module

class XBoyEmulator {
    constructor() {
        this.emulator = null;
        this.isRunning = false;
        this.isPaused = false;
        this.currentScale = 3;
        this.romData = null;
        this.audioContext = null;
        this.frameInterval = null;
        this.lastFrameTime = 0;
        this.fps = 0;
        this.frameCount = 0;
        this.fpsUpdateInterval = null;
        
        this.initializeEventListeners();
    }

    async initialize() {
        this.showLoading(true);
        
        try {
            // Initialize WebAudio context
            this.audioContext = new (window.AudioContext || window.webkitAudioContext)();
            
            // Wait for WebAssembly module to load
            this.showLoading(false);
            return true;
        } catch (error) {
            console.error('Failed to initialize emulator:', error);
            this.showError('Failed to initialize emulator: ' + error.message);
            this.showLoading(false);
            return false;
        }
    }

    async loadRom(file) {
        try {
            console.log('Loading ROM:', file.name, 'Size:', file.size);
            this.romData = await this.readFileAsArray(file);
            console.log('ROM data loaded, length:', this.romData.length);
            this.updateRomName(file.name);
            this.enableEmulationControls();
            
            // Try to create emulator instance
            if (window.XBoyWASM) {
                console.log('XBoyWASM module found, loading...');
                const module = await window.XBoyWASM({
                    canvas: document.getElementById('game-canvas'),
                    doNotCaptureKeyboard: true
                });
                console.log('Module loaded, writing ROM to virtual FS...');
                // Write ROM bytes directly to Emscripten's virtual filesystem
                // to avoid UTF-8 corruption from embind string conversion
                const romPath = '/tmp/temp_rom.gb';
                module.FS.writeFile(romPath, this.romData);
                console.log('ROM written to virtual FS, creating WebEmulator instance...');
                this.emulator = new module.WebEmulator(romPath);
                console.log('WebEmulator created successfully');
                this.showGameDisplay();
                this.startEmulation();
                return true;
            } else {
                console.error('XBoyWASM module not found');
                this.showError('WebAssembly module not loaded. Please wait and try again.');
                return false;
            }
        } catch (error) {
            console.error('Failed to load ROM:', error);
            console.error('Error stack:', error.stack);
            this.showError('Failed to load ROM: ' + error.message);
            return false;
        }
    }

    readFileAsArray(file) {
        return new Promise((resolve, reject) => {
            const reader = new FileReader();
            reader.onload = () => {
                const arrayBuffer = reader.result;
                const uint8Array = new Uint8Array(arrayBuffer);
                resolve(uint8Array);
            };
            reader.onerror = reject;
            reader.readAsArrayBuffer(file);
        });
    }

    startEmulation() {
        if (!this.emulator || this.isRunning) return;
        
        this.isRunning = true;
        this.isPaused = false;
        this.updateButtonStates();
        this.startAudioContext();
        this.startFPSCounter();
        this.runEmulationLoop();
    }

    pauseEmulation() {
        if (!this.isRunning) return;
        
        this.isPaused = true;
        this.isRunning = false;
        this.updateButtonStates();
        this.stopFPSCounter();
        
        if (this.frameInterval) {
            cancelAnimationFrame(this.frameInterval);
            this.frameInterval = null;
        }
    }

    resetEmulation() {
        if (!this.emulator) return;
        
        this.pauseEmulation();
        if (this.romData) {
            this.loadRomFromData(this.romData);
        }
    }

    async loadRomFromData(romData) {
        try {
            if (window.XBoyWASM) {
                const module = await window.XBoyWASM({
                    canvas: document.getElementById('game-canvas'),
                    doNotCaptureKeyboard: true
                });
                const romPath = '/tmp/temp_rom.gb';
                module.FS.writeFile(romPath, romData);
                this.emulator = new module.WebEmulator(romPath);
            }
        } catch (error) {
            console.error('Failed to reset ROM:', error);
            this.showError('Failed to reset ROM: ' + error.message);
        }
    }

    runEmulationLoop() {
        if (!this.isRunning) return;
        
        const now = performance.now();
        const targetFrameTime = 1000 / 60; // 60 FPS
        
        if (now - this.lastFrameTime >= targetFrameTime) {
            if (this.emulator) {
                this.emulator.stepFrame();
            }
            
            this.frameCount++;
            this.lastFrameTime = now;
        }
        
        this.frameInterval = requestAnimationFrame(() => this.runEmulationLoop());
    }

    startFPSCounter() {
        this.frameCount = 0;
        this.fpsUpdateInterval = setInterval(() => {
            this.fps = this.frameCount;
            this.frameCount = 0;
            this.updateFPSDisplay();
        }, 1000);
    }

    stopFPSCounter() {
        if (this.fpsUpdateInterval) {
            clearInterval(this.fpsUpdateInterval);
            this.fpsUpdateInterval = null;
        }
    }

    updateFPSDisplay() {
        const fpsCounter = document.getElementById('fps-counter');
        if (fpsCounter) {
            fpsCounter.textContent = `${this.fps} FPS`;
        }
    }

    startAudioContext() {
        if (this.audioContext && this.audioContext.state === 'suspended') {
            this.audioContext.resume();
        }
    }

    setScale(scale) {
        this.currentScale = scale;
        this.updateCanvasScale();
    }

    updateCanvasScale() {
        const canvas = document.getElementById('game-canvas');
        if (canvas) {
            canvas.style.width = (160 * this.currentScale) + 'px';
            canvas.style.height = (144 * this.currentScale) + 'px';
        }
    }

    updateButtonStates() {
        document.getElementById('start-emulation').disabled = this.isRunning;
        document.getElementById('pause-emulation').disabled = !this.isRunning;
        document.getElementById('reset-emulation').disabled = !this.emulator;
    }

    updateRomName(name) {
        const romName = document.getElementById('rom-name');
        if (romName) {
            romName.textContent = name || 'No ROM loaded';
        }
    }

    showGameDisplay() {
        document.getElementById('drop-zone').style.display = 'none';
        document.getElementById('game-display').style.display = 'block';
    }

    showDropZone() {
        document.getElementById('drop-zone').style.display = 'block';
        document.getElementById('game-display').style.display = 'none';
    }

    showLoading(show) {
        const overlay = document.getElementById('loading-overlay');
        if (overlay) {
            overlay.style.display = show ? 'flex' : 'none';
        }
    }

    showError(message) {
        alert('Error: ' + message);
    }

    enableEmulationControls() {
        document.getElementById('start-emulation').disabled = false;
        document.getElementById('reset-emulation').disabled = false;
    }

    initializeEventListeners() {
        // ROM loading buttons
        document.getElementById('load-rom').addEventListener('click', () => {
            document.getElementById('rom-file').click();
        });

        document.getElementById('browse-rom').addEventListener('click', () => {
            document.getElementById('rom-file').click();
        });

        document.getElementById('rom-file').addEventListener('change', (e) => {
            const file = e.target.files[0];
            if (file) {
                this.loadRom(file);
            }
        });

        // Emulation controls
        document.getElementById('start-emulation').addEventListener('click', () => {
            this.startEmulation();
        });

        document.getElementById('pause-emulation').addEventListener('click', () => {
            this.pauseEmulation();
        });

        document.getElementById('reset-emulation').addEventListener('click', () => {
            this.resetEmulation();
        });

        // Settings
        document.getElementById('scale').addEventListener('change', (e) => {
            this.setScale(parseInt(e.target.value));
        });

        // Keyboard input is handled by ControlsManager in controls.js
    }

    // Button mapping for virtual controls
    getButtonMap() {
        return {
            'Up': 0,
            'Down': 1,
            'Left': 2,
            'Right': 3,
            'A': 4,
            'B': 5,
            'Start': 6,
            'Select': 7
        };
    }
}

// Initialize the emulator when the page loads
document.addEventListener('DOMContentLoaded', async () => {
    const emulator = new XBoyEmulator();
    
    // Initialize the emulator
    await emulator.initialize();
    
    // Initialize ROM loader
    const romLoader = new ROMLoader(emulator);
    
    // Initialize save state manager
    const saveStateManager = new SaveStateManager(emulator);
    
    // Initialize controls manager
    const controlsManager = new ControlsManager(emulator);
    
    // Initialize gamepad support if available
    controlsManager.initializeGamepadSupport();
    
    // Set initial scale
    emulator.setScale(3);
    
    // Make modules globally accessible for other scripts
    window.xboyEmulator = emulator;
    window.romLoader = romLoader;
    window.saveStateManager = saveStateManager;
    window.controlsManager = controlsManager;
    
    console.log('XBoy Emulator initialized');
});