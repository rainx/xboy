// Save States Manager - Handles browser-based save state management

class SaveStateManager {
    constructor(emulator) {
        this.emulator = emulator;
        this.storageKey = 'xboy_save_states';
        this.maxSlots = 4;
        this.currentSlot = 1;
        this.saveStates = new Map();
        
        this.initializeUI();
        this.loadSaveStatesFromStorage();
    }

    initializeUI() {
        // Save slot buttons
        document.querySelectorAll('.save-slot').forEach((btn, index) => {
            const slot = index + 1;
            btn.addEventListener('click', () => this.selectSlot(slot));
            btn.dataset.slot = slot;
        });

        // Save/Load buttons
        document.getElementById('save-state').addEventListener('click', () => this.saveState());
        document.getElementById('load-state').addEventListener('click', () => this.loadState());
    }

    selectSlot(slot) {
        this.currentSlot = slot;
        this.updateSlotDisplay();
    }

    updateSlotDisplay() {
        document.querySelectorAll('.save-slot').forEach((btn, index) => {
            const slot = index + 1;
            
            if (slot === this.currentSlot) {
                btn.classList.add('active');
            } else {
                btn.classList.remove('active');
            }
            
            // Show slot info
            const saveState = this.saveStates.get(slot);
            if (saveState) {
                btn.title = `Slot ${slot}: ${saveState.name} - ${new Date(saveState.timestamp).toLocaleString()}`;
                btn.style.background = this.hasDataInSlot(slot) ? 'rgba(102, 126, 234, 0.3)' : '';
            } else {
                btn.title = `Slot ${slot}: Empty`;
                btn.style.background = '';
            }
        });
    }

    async saveState() {
        if (!this.emulator.emulator) {
            this.showNotification('No ROM loaded', 'error');
            return;
        }

        try {
            this.emulator.showLoading(true);
            
            // Call C++ saveState method
            const success = this.emulator.emulator.saveState(this.currentSlot);
            
            if (success) {
                // Get current ROM name
                const romName = document.getElementById('rom-name').textContent;
                
                // Create save state metadata
                const saveState = {
                    slot: this.currentSlot,
                    name: romName,
                    timestamp: Date.now(),
                    screenshot: this.captureScreenshot()
                };
                
                // Store in memory
                this.saveStates.set(this.currentSlot, saveState);
                
                // Save to browser storage
                await this.saveToBrowserStorage();
                
                // Update UI
                this.updateSlotDisplay();
                this.showNotification(`State saved to slot ${this.currentSlot}`, 'success');
            } else {
                this.showNotification('Failed to save state', 'error');
            }
            
        } catch (error) {
            console.error('Save state error:', error);
            this.showNotification('Failed to save state: ' + error.message, 'error');
        } finally {
            this.emulator.showLoading(false);
        }
    }

    async loadState() {
        if (!this.emulator.emulator) {
            this.showNotification('No ROM loaded', 'error');
            return;
        }

        const saveState = this.saveStates.get(this.currentSlot);
        if (!saveState) {
            this.showNotification(`No save state in slot ${this.currentSlot}`, 'error');
            return;
        }

        try {
            this.emulator.showLoading(true);
            
            // Call C++ loadState method
            const success = this.emulator.emulator.loadState(this.currentSlot);
            
            if (success) {
                this.showNotification(`State loaded from slot ${this.currentSlot}`, 'success');
            } else {
                this.showNotification('Failed to load state', 'error');
            }
            
        } catch (error) {
            console.error('Load state error:', error);
            this.showNotification('Failed to load state: ' + error.message, 'error');
        } finally {
            this.emulator.showLoading(false);
        }
    }

    async saveToBrowserStorage() {
        try {
            // Convert Map to Array for storage
            const saveStatesArray = Array.from(this.saveStates.entries()).map(([slot, data]) => ({
                slot,
                ...data
            }));
            
            // Save to localStorage (for small save states)
            if (this.canUseLocalStorage()) {
                localStorage.setItem(this.storageKey, JSON.stringify(saveStatesArray));
            }
            
            // Also save to IndexedDB for larger save states
            await this.saveToIndexedDB(saveStatesArray);
            
        } catch (error) {
            console.error('Failed to save to browser storage:', error);
            throw error;
        }
    }

    async loadFromBrowserStorage() {
        try {
            // Try IndexedDB first (for larger save states)
            const indexedDBData = await this.loadFromIndexedDB();
            if (indexedDBData) {
                this.loadSaveStatesFromArray(indexedDBData);
                return;
            }
            
            // Fallback to localStorage
            if (this.canUseLocalStorage()) {
                const localStorageData = localStorage.getItem(this.storageKey);
                if (localStorageData) {
                    const saveStatesArray = JSON.parse(localStorageData);
                    this.loadSaveStatesFromArray(saveStatesArray);
                }
            }
            
        } catch (error) {
            console.error('Failed to load from browser storage:', error);
        }
    }

    loadSaveStatesFromArray(saveStatesArray) {
        this.saveStates.clear();
        
        saveStatesArray.forEach(saveState => {
            this.saveStates.set(saveState.slot, {
                slot: saveState.slot,
                name: saveState.name,
                timestamp: saveState.timestamp,
                screenshot: saveState.screenshot
            });
        });
        
        this.updateSlotDisplay();
    }

    loadSaveStatesFromStorage() {
        this.loadFromBrowserStorage();
    }

    canUseLocalStorage() {
        try {
            const test = '__test__';
            localStorage.setItem(test, test);
            localStorage.removeItem(test);
            return true;
        } catch (e) {
            return false;
        }
    }

    async saveToIndexedDB(saveStatesArray) {
        return new Promise((resolve, reject) => {
            const request = indexedDB.open('XBoySaveStates', 1);
            
            request.onerror = () => reject(request.error);
            request.onsuccess = () => {
                const db = request.result;
                const transaction = db.transaction(['saveStates'], 'readwrite');
                const store = transaction.objectStore('saveStates');
                
                // Clear existing data
                store.clear();
                
                // Add new data
                saveStatesArray.forEach(saveState => {
                    store.put(saveState, `slot_${saveState.slot}`);
                });
                
                transaction.oncomplete = () => resolve();
                transaction.onerror = () => reject(transaction.error);
            };
            
            request.onupgradeneeded = (event) => {
                const db = event.target.result;
                if (!db.objectStoreNames.contains('saveStates')) {
                    db.createObjectStore('saveStates');
                }
            };
        });
    }

    async loadFromIndexedDB() {
        return new Promise((resolve, reject) => {
            const request = indexedDB.open('XBoySaveStates', 1);
            
            request.onerror = () => reject(request.error);
            request.onsuccess = () => {
                const db = request.result;
                const transaction = db.transaction(['saveStates'], 'readonly');
                const store = transaction.objectStore('saveStates');
                
                const getAllRequest = store.getAll();
                
                getAllRequest.onsuccess = () => {
                    resolve(getAllRequest.result);
                };
                
                getAllRequest.onerror = () => reject(getAllRequest.error);
            };
            
            request.onupgradeneeded = (event) => {
                const db = event.target.result;
                if (!db.objectStoreNames.contains('saveStates')) {
                    db.createObjectStore('saveStates');
                }
            };
        });
    }

    captureScreenshot() {
        const canvas = document.getElementById('game-canvas');
        if (!canvas) return null;
        
        return canvas.toDataURL('image/png', 0.8);
    }

    hasDataInSlot(slot) {
        return this.saveStates.has(slot);
    }

    getSaveStateInfo(slot) {
        return this.saveStates.get(slot);
    }

    clearAllSaveStates() {
        this.saveStates.clear();
        
        // Clear localStorage
        if (this.canUseLocalStorage()) {
            localStorage.removeItem(this.storageKey);
        }
        
        // Clear IndexedDB
        this.clearIndexedDB();
        
        this.updateSlotDisplay();
        this.showNotification('All save states cleared', 'success');
    }

    async clearIndexedDB() {
        return new Promise((resolve, reject) => {
            const request = indexedDB.open('XBoySaveStates', 1);
            
            request.onerror = () => reject(request.error);
            request.onsuccess = () => {
                const db = request.result;
                const transaction = db.transaction(['saveStates'], 'readwrite');
                const store = transaction.objectStore('saveStates');
                
                store.clear();
                
                transaction.oncomplete = () => resolve();
                transaction.onerror = () => reject(transaction.error);
            };
        });
    }

    showNotification(message, type = 'info') {
        // Create notification element
        const notification = document.createElement('div');
        notification.className = `notification ${type}`;
        notification.textContent = message;
        
        // Style the notification
        Object.assign(notification.style, {
            position: 'fixed',
            bottom: '20px',
            right: '20px',
            padding: '15px 20px',
            borderRadius: '8px',
            color: 'white',
            fontWeight: 'bold',
            zIndex: '3000',
            opacity: '0',
            transform: 'translateY(20px)',
            transition: 'all 0.3s ease'
        });

        // Set background color based on type
        switch (type) {
            case 'success':
                notification.style.background = 'linear-gradient(135deg, #4CAF50, #45a049)';
                break;
            case 'error':
                notification.style.background = 'linear-gradient(135deg, #f44336, #da190b)';
                break;
            default:
                notification.style.background = 'linear-gradient(135deg, #2196F3, #0b7dda)';
        }

        // Add to document
        document.body.appendChild(notification);

        // Animate in
        setTimeout(() => {
            notification.style.opacity = '1';
            notification.style.transform = 'translateY(0)';
        }, 10);

        // Remove after 3 seconds
        setTimeout(() => {
            notification.style.opacity = '0';
            notification.style.transform = 'translateY(20px)';
            
            setTimeout(() => {
                if (notification.parentNode) {
                    notification.parentNode.removeChild(notification);
                }
            }, 300);
        }, 3000);
    }
}