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