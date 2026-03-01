// ROM Loader Module - Handles drag-and-drop and file picker functionality

class ROMLoader {
    constructor(emulator) {
        this.emulator = emulator;
        this.dropZone = document.getElementById('drop-zone');
        this.fileInput = document.getElementById('rom-file');
        this.supportedExtensions = ['.gb', '.gbc'];
        this.maxFileSize = 8 * 1024 * 1024; // 8MB max
        
        this.initializeDropZone();
        this.initializeFileInput();
    }

    initializeDropZone() {
        if (!this.dropZone) return;

        ['dragenter', 'dragover', 'dragleave', 'drop'].forEach(eventName => {
            this.dropZone.addEventListener(eventName, this.preventDefaults, false);
        });

        ['dragenter', 'dragover'].forEach(eventName => {
            this.dropZone.addEventListener(eventName, () => {
                this.dropZone.classList.add('drag-over');
            }, false);
        });

        ['dragleave', 'drop'].forEach(eventName => {
            this.dropZone.addEventListener(eventName, () => {
                this.dropZone.classList.remove('drag-over');
            }, false);
        });

        this.dropZone.addEventListener('drop', this.handleDrop.bind(this), false);
    }

    initializeFileInput() {
        if (!this.fileInput) return;
        
        this.fileInput.addEventListener('change', this.handleFileSelect.bind(this));
    }

    preventDefaults(e) {
        e.preventDefault();
        e.stopPropagation();
    }

    async handleDrop(e) {
        const dt = e.dataTransfer;
        const files = dt.files;

        if (files.length > 0) {
            await this.processFile(files[0]);
        }
    }

    async handleFileSelect(e) {
        const files = e.target.files;
        
        if (files.length > 0) {
            await this.processFile(files[0]);
        }
    }

    async processFile(file) {
        try {
            // Validate file
            if (!this.validateFile(file)) {
                return;
            }

            this.emulator.showLoading(true);
            
            // Load ROM
            const success = await this.emulator.loadRom(file);
            
            if (success) {
                this.showSuccess('ROM loaded successfully!');
            } else {
                this.showError('Failed to load ROM');
            }
            
            this.emulator.showLoading(false);
            
        } catch (error) {
            console.error('Error processing file:', error);
            this.showError('Error processing ROM: ' + error.message);
            this.emulator.showLoading(false);
        }
    }

    validateFile(file) {
        // Check file extension
        const extension = this.getFileExtension(file.name);
        if (!this.supportedExtensions.includes(extension.toLowerCase())) {
            this.showError('Invalid file type. Please select a .gb or .gbc file.');
            return false;
        }

        // Check file size
        if (file.size > this.maxFileSize) {
            this.showError('File too large. Maximum size is 8MB.');
            return false;
        }

        return true;
    }

    getFileExtension(filename) {
        return filename.substring(filename.lastIndexOf('.'));
    }

    readFileAsArray(file) {
        return new Promise((resolve, reject) => {
            const reader = new FileReader();
            
            reader.onload = () => {
                if (reader.readyState === FileReader.DONE) {
                    resolve(reader.result);
                }
            };
            
            reader.onerror = () => {
                reject(new Error('Failed to read file'));
            };
            
            reader.readAsBinaryString(file);
        });
    }

    showSuccess(message) {
        this.showNotification(message, 'success');
    }

    showError(message) {
        this.showNotification(message, 'error');
    }

    showNotification(message, type = 'info') {
        const notification = document.createElement('div');
        notification.className = `notification ${type}`;
        notification.textContent = message;

        const colors = {
            success: '#1d1d1f',
            error: '#ff3b30',
            info: '#1d1d1f'
        };

        Object.assign(notification.style, {
            position: 'fixed',
            top: '16px',
            right: '16px',
            padding: '10px 18px',
            borderRadius: '10px',
            color: '#fff',
            fontSize: '13px',
            fontWeight: '500',
            fontFamily: '-apple-system, BlinkMacSystemFont, sans-serif',
            zIndex: '3000',
            opacity: '0',
            transform: 'translateY(-8px) scale(0.98)',
            transition: 'all 0.25s cubic-bezier(0.25, 0.1, 0.25, 1)',
            background: colors[type] || colors.info,
            backdropFilter: 'blur(12px)',
            WebkitBackdropFilter: 'blur(12px)',
            boxShadow: '0 4px 16px rgba(0,0,0,0.12)'
        });

        document.body.appendChild(notification);

        requestAnimationFrame(() => {
            notification.style.opacity = '1';
            notification.style.transform = 'translateY(0) scale(1)';
        });

        setTimeout(() => {
            notification.style.opacity = '0';
            notification.style.transform = 'translateY(-8px) scale(0.98)';
            setTimeout(() => notification.remove(), 250);
        }, 2500);
    }

    // Utility method to check if file is a valid ROM
    static isROMFile(file) {
        const extension = file.name.substring(file.name.lastIndexOf('.')).toLowerCase();
        return ['.gb', '.gbc'].includes(extension);
    }

    // Utility method to get ROM info from file
    static async getROMInfo(file) {
        try {
            const romData = await this.readFileAsArray(file);
            
            // Extract basic ROM info from header
            if (romData.length >= 0x150) {
                const titleBytes = romData.slice(0x134, 0x144);
                const title = String.fromCharCode(...titleBytes.filter(b => b !== 0));
                
                const gbcFlag = romData[0x143];
                const isGBC = (gbcFlag & 0x80) === 0x80;
                
                return {
                    title: title.trim(),
                    isGBC: isGBC,
                    size: file.size,
                    format: isGBC ? 'GBC' : 'GB'
                };
            }
            
            return {
                title: file.name,
                isGBC: false,
                size: file.size,
                format: 'Unknown'
            };
        } catch (error) {
            console.error('Failed to read ROM info:', error);
            return {
                title: file.name,
                isGBC: false,
                size: file.size,
                format: 'Unknown'
            };
        }
    }
}