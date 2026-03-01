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
        // Create notification element
        const notification = document.createElement('div');
        notification.className = `notification ${type}`;
        notification.textContent = message;
        
        // Style the notification
        Object.assign(notification.style, {
            position: 'fixed',
            top: '20px',
            right: '20px',
            padding: '15px 20px',
            borderRadius: '8px',
            color: 'white',
            fontWeight: 'bold',
            zIndex: '3000',
            opacity: '0',
            transform: 'translateY(-20px)',
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
            notification.style.transform = 'translateY(-20px)';
            
            setTimeout(() => {
                if (notification.parentNode) {
                    notification.parentNode.removeChild(notification);
                }
            }, 300);
        }, 3000);
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