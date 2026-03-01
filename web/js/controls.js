// Controls Module - Handles keyboard and virtual controls

class ControlsManager {
    constructor(emulator) {
        this.emulator = emulator;
        this.virtualControls = document.getElementById('virtual-controls');
        this.isMobile = this.detectMobile();
        this.keyMap = this.getKeyMap();
        this.virtualButtonMap = this.getVirtualButtonMap();
        this.pressedKeys = new Set();
        this.pressedVirtualButtons = new Set();
        
        this.initializeEventListeners();
        this.setupMobileControls();
    }

    detectMobile() {
        return /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent) ||
               (window.innerWidth <= 768 && 'ontouchstart' in window);
    }

    getKeyMap() {
        // Map directly from key to C++ enum input::Button integer value
        // C++ enum: Right=0, Left=1, Up=2, Down=3, A=4, B=5, Select=6, Start=7
        return {
            'ArrowRight': 0,
            'ArrowLeft': 1,
            'ArrowUp': 2,
            'ArrowDown': 3,
            'z': 4,
            'x': 5,
            'Z': 4,
            'X': 5,
            'Backspace': 6,
            'Enter': 7,
            ' ': 7
        };
    }

    getVirtualButtonMap() {
        return {
            'virtual-up': 'Up',
            'virtual-down': 'Down',
            'virtual-left': 'Left',
            'virtual-right': 'Right',
            'virtual-a': 'A',
            'virtual-b': 'B',
            'virtual-start': 'Start',
            'virtual-select': 'Select'
        };
    }

    initializeEventListeners() {
        // Keyboard events
        document.addEventListener('keydown', this.handleKeyDown.bind(this));
        document.addEventListener('keyup', this.handleKeyUp.bind(this));
        
        // Prevent default behavior for game keys
        document.addEventListener('keydown', this.preventDefaultKeys.bind(this));
        
        // Handle window focus/blur
        window.addEventListener('blur', this.handleWindowBlur.bind(this));
        window.addEventListener('focus', this.handleWindowFocus.bind(this));
        
        // Handle page visibility
        document.addEventListener('visibilitychange', this.handleVisibilityChange.bind(this));
    }

    setupMobileControls() {
        if (this.isMobile) {
            this.virtualControls.style.display = 'flex';
            this.initializeVirtualControls();
        } else {
            this.virtualControls.style.display = 'none';
        }
    }

    initializeVirtualControls() {
        // Virtual D-pad
        const dPadButtons = this.virtualControls.querySelectorAll('.d-pad-btn');
        dPadButtons.forEach(button => {
            // Touch events
            button.addEventListener('touchstart', this.handleVirtualTouchStart.bind(this));
            button.addEventListener('touchend', this.handleVirtualTouchEnd.bind(this));
            button.addEventListener('touchmove', this.handleVirtualTouchMove.bind(this));
            
            // Mouse events (for testing)
            button.addEventListener('mousedown', this.handleVirtualMouseDown.bind(this));
            button.addEventListener('mouseup', this.handleVirtualMouseUp.bind(this));
        });

        // Virtual action buttons
        const actionButtons = this.virtualControls.querySelectorAll('.action-btn');
        actionButtons.forEach(button => {
            // Touch events
            button.addEventListener('touchstart', this.handleVirtualTouchStart.bind(this));
            button.addEventListener('touchend', this.handleVirtualTouchEnd.bind(this));
            button.addEventListener('touchmove', this.handleVirtualTouchMove.bind(this));
            
            // Mouse events (for testing)
            button.addEventListener('mousedown', this.handleVirtualMouseDown.bind(this));
            button.addEventListener('mouseup', this.handleVirtualMouseUp.bind(this));
        });
    }

    handleKeyDown(e) {
        const buttonIndex = this.keyMap[e.key];
        if (buttonIndex === undefined) return;

        // Prevent repeat key events
        if (this.pressedKeys.has(buttonIndex)) return;

        console.log('KeyDown:', e.key, '→ button index:', buttonIndex);
        this.pressedKeys.add(buttonIndex);
        this.sendButtonIndex(buttonIndex, true);
        e.preventDefault();
    }

    handleKeyUp(e) {
        const buttonIndex = this.keyMap[e.key];
        if (buttonIndex === undefined) return;

        this.pressedKeys.delete(buttonIndex);
        this.sendButtonIndex(buttonIndex, false);
        e.preventDefault();
    }

    preventDefaultKeys(e) {
        if (this.keyMap[e.key] !== undefined) {
            e.preventDefault();
        }
    }

    handleVirtualTouchStart(e) {
        e.preventDefault();
        const buttonId = e.target.id;
        const button = this.virtualButtonMap[buttonId];
        
        if (button) {
            this.pressedVirtualButtons.add(button);
            this.sendButtonState(button, true);
            e.target.classList.add('active');
        }
    }

    handleVirtualTouchEnd(e) {
        e.preventDefault();
        const buttonId = e.target.id;
        const button = this.virtualButtonMap[buttonId];
        
        if (button) {
            this.pressedVirtualButtons.delete(button);
            this.sendButtonState(button, false);
            e.target.classList.remove('active');
        }
    }

    handleVirtualTouchMove(e) {
        e.preventDefault();
        // Handle D-pad swipe gestures
        if (e.target.classList.contains('d-pad-btn')) {
            this.handleDPadSwipe(e);
        }
    }

    handleVirtualMouseDown(e) {
        const buttonId = e.target.id;
        const button = this.virtualButtonMap[buttonId];
        
        if (button) {
            this.pressedVirtualButtons.add(button);
            this.sendButtonState(button, true);
            e.target.classList.add('active');
        }
    }

    handleVirtualMouseUp(e) {
        const buttonId = e.target.id;
        const button = this.virtualButtonMap[buttonId];
        
        if (button) {
            this.pressedVirtualButtons.delete(button);
            this.sendButtonState(button, false);
            e.target.classList.remove('active');
        }
    }

    handleDPadSwipe(e) {
        // Simple swipe detection for D-pad
        if (e.touches.length !== 1) return;
        
        const touch = e.touches[0];
        const rect = e.target.getBoundingClientRect();
        const x = touch.clientX - rect.left;
        const y = touch.clientY - rect.top;
        
        // Determine direction based on touch position
        const centerX = rect.width / 2;
        const centerY = rect.height / 2;
        
        // Clear all directions first
        ['Up', 'Down', 'Left', 'Right'].forEach(dir => {
            if (this.pressedVirtualButtons.has(dir)) {
                this.sendButtonState(dir, false);
                this.pressedVirtualButtons.delete(dir);
            }
        });
        
        // Set new direction
        let direction = null;
        if (Math.abs(x - centerX) > Math.abs(y - centerY)) {
            direction = x > centerX ? 'Right' : 'Left';
        } else {
            direction = y > centerY ? 'Down' : 'Up';
        }
        
        if (direction) {
            this.pressedVirtualButtons.add(direction);
            this.sendButtonState(direction, true);
        }
    }

    handleWindowBlur() {
        // Release all keys when window loses focus
        this.releaseAllButtons();
    }

    handleWindowFocus() {
        // Reset key state when window gains focus
        this.pressedKeys.clear();
    }

    handleVisibilityChange() {
        if (document.hidden) {
            // Pause emulation when page is hidden
            if (this.emulator && this.emulator.isRunning) {
                this.emulator.pauseEmulation();
            }
        }
    }

    sendButtonIndex(buttonIndex, pressed) {
        if (!this.emulator.emulator) return;
        this.emulator.emulator.setButtonState(buttonIndex, pressed);
    }

    sendButtonState(button, pressed) {
        if (!this.emulator.emulator) return;
        // Used by virtual controls and gamepad (which still use string names)
        const buttonIndex = this.getButtonIndex(button);
        if (buttonIndex !== -1) {
            this.emulator.emulator.setButtonState(buttonIndex, pressed);
        }
    }

    getButtonIndex(button) {
        // Must match C++ enum input::Button
        // Right=0, Left=1, Up=2, Down=3, A=4, B=5, Select=6, Start=7
        const buttonMap = {
            'Right': 0,
            'Left': 1,
            'Up': 2,
            'Down': 3,
            'A': 4,
            'B': 5,
            'Select': 6,
            'Start': 7
        };
        const index = buttonMap[button];
        return index !== undefined ? index : -1;
    }

    releaseAllButtons() {
        // Release all keyboard buttons (now stored as integer indices)
        this.pressedKeys.forEach(buttonIndex => {
            this.sendButtonIndex(buttonIndex, false);
        });
        this.pressedKeys.clear();
        
        // Release all virtual buttons
        this.pressedVirtualButtons.forEach(button => {
            this.sendButtonState(button, false);
        });
        this.pressedVirtualButtons.clear();
        
        // Update visual state
        document.querySelectorAll('.d-pad-btn, .action-btn').forEach(btn => {
            btn.classList.remove('active');
        });
    }

    // Add support for game controller API
    initializeGamepadSupport() {
        if (!('getGamepads' in navigator)) return;
        
        window.addEventListener('gamepadconnected', (e) => {
            console.log('Gamepad connected:', e.gamepad);
        });
        
        window.addEventListener('gamepaddisconnected', (e) => {
            console.log('Gamepad disconnected:', e.gamepad);
        });
        
        // Poll gamepads regularly
        setInterval(() => {
            this.updateGamepadState();
        }, 16); // ~60 FPS
    }

    updateGamepadState() {
        const gamepads = navigator.getGamepads();
        
        for (let i = 0; i < gamepads.length; i++) {
            const gamepad = gamepads[i];
            if (!gamepad) continue;
            
            this.processGamepad(gamepad);
        }
    }

    processGamepad(gamepad) {
        // Map gamepad buttons to Game Boy controls
        const buttonMapping = [
            { index: 0, button: 'A' },      // A button (often X on Xbox)
            { index: 1, button: 'B' },      // B button (often B on Xbox)
            { index: 2, button: 'Select' },  // Select
            { index: 3, button: 'Start' },   // Start
            { index: 12, button: 'Up' },     // D-pad Up
            { index: 13, button: 'Down' },   // D-pad Down
            { index: 14, button: 'Left' },   // D-pad Left
            { index: 15, button: 'Right' }   // D-pad Right
        ];
        
        buttonMapping.forEach(mapping => {
            const pressed = gamepad.buttons[mapping.index].pressed;
            this.sendButtonState(mapping.button, pressed);
        });
        
        // Handle analog stick as D-pad
        const deadzone = 0.5;
        if (gamepad.axes.length >= 2) {
            const xAxis = gamepad.axes[0];
            const yAxis = gamepad.axes[1];
            
            // Horizontal
            if (Math.abs(xAxis) > deadzone) {
                const left = xAxis < -deadzone;
                const right = xAxis > deadzone;
                this.sendButtonState('Left', left);
                this.sendButtonState('Right', right);
            } else {
                this.sendButtonState('Left', false);
                this.sendButtonState('Right', false);
            }
            
            // Vertical
            if (Math.abs(yAxis) > deadzone) {
                const up = yAxis < -deadzone;
                const down = yAxis > deadzone;
                this.sendButtonState('Up', up);
                this.sendButtonState('Down', down);
            } else {
                this.sendButtonState('Up', false);
                this.sendButtonState('Down', false);
            }
        }
    }

    // Show/hide virtual controls based on device type
    updateVirtualControlsVisibility() {
        if (this.isMobile) {
            this.virtualControls.style.display = 'flex';
        } else {
            this.virtualControls.style.display = 'none';
        }
    }

    // Add visual feedback for controls
    addVisualFeedback() {
        // Add active state styles for virtual buttons
        const style = document.createElement('style');
        style.textContent = `
            .d-pad-btn.active, .action-btn.active {
                background: rgba(102, 126, 234, 0.8) !important;
                border-color: #667eea !important;
                transform: scale(0.95);
            }
        `;
        document.head.appendChild(style);
    }
}