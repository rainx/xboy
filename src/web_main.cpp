#ifdef EMSCRIPTEN

#include <emscripten.h>
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
#include <memory>
#include <string>
#include <cstdio>

class WebEmulator {
public:
    // Accept a file path to a ROM already written to Emscripten's virtual FS.
    // Binary data must NOT be passed as std::string through embind, because
    // embind encodes JS strings as UTF-8, corrupting any byte >= 0x80.
    WebEmulator(const std::string& romPath) {
        printf("WebEmulator: Constructor called, ROM path: %s\n", romPath.c_str());
        tempRomPath_ = romPath;
        
        try {
            printf("WebEmulator: Initializing MMU...\n");
            mmu_ = mmu::Mmunit::powerUp(tempRomPath_);
            printf("WebEmulator: MMU initialized successfully\n");
            
            printf("WebEmulator: Initializing CPU...\n");
            cpu_ = std::make_unique<cpu::Cpu>(mmu_);
            printf("WebEmulator: CPU initialized successfully\n");
            
            printf("WebEmulator: Initializing Timer...\n");
            timer_ = std::make_unique<timer::Timer>(mmu_);
            printf("WebEmulator: Timer initialized successfully\n");
            
            printf("WebEmulator: Initializing PPU...\n");
            ppu_ = std::make_unique<ppu::Ppu>(mmu_);
            printf("WebEmulator: PPU initialized successfully\n");
            
            printf("WebEmulator: Initializing APU...\n");
            apu_ = std::make_unique<apu::Apu>(mmu_);
            printf("WebEmulator: APU initialized successfully\n");
            
            printf("WebEmulator: Initializing Joypad...\n");
            joypad_ = std::make_shared<input::Joypad>(mmu_);
            printf("WebEmulator: Joypad initialized successfully\n");
            
            printf("WebEmulator: Initializing Display...\n");
            display_ = std::make_unique<platform::WebDisplay>(1);
            printf("WebEmulator: Display initialized successfully\n");
            
            printf("WebEmulator: Initializing Audio...\n");
            audio_ = std::make_unique<platform::WebAudio>();
            printf("WebEmulator: Audio initialized successfully\n");
            
            printf("WebEmulator: Initializing Input...\n");
            input_ = std::make_unique<platform::WebInput>(joypad_);
            printf("WebEmulator: Input initialized successfully\n");
            
            printf("WebEmulator: Setting up audio callback...\n");
            setupAudioCallback();
            printf("WebEmulator: Audio callback setup successfully\n");
            
        } catch (const std::exception& e) {
            std::string error = std::string("Exception during initialization: ") + e.what();
            printf("WebEmulator: ERROR: %s\n", error.c_str());
            throw std::runtime_error(error);
        } catch (...) {
            printf("WebEmulator: ERROR: Unknown exception during initialization\n");
            throw std::runtime_error("Unknown exception during initialization");
        }
        
        printf("WebEmulator: Initialization complete\n");
    }
    
    ~WebEmulator() {
        std::remove(tempRomPath_.c_str());
    }
    
    void stepFrame() {
        uint32_t frame_cycles = 0;
        while (frame_cycles < 70224) {
            uint8_t cycles = cpu_->step();
            timer_->step(cycles);
            ppu_->step(cycles);
            apu_->step(cycles);
            frame_cycles += cycles;
            
            uint8_t int_cycles = cpu_->handleInterrupts();
            frame_cycles += int_cycles;
            
            joypad_->update();
        }
        
        if (ppu_->frameReady()) {
            display_->render(ppu_->getFrameBuffer());
            ppu_->clearFrameReady();
        }
        
        input_->poll();
    }
    
    void setButtonState(int button, bool pressed) {
        if (pressed) {
            joypad_->press(static_cast<input::Button>(button));
        } else {
            joypad_->release(static_cast<input::Button>(button));
        }
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

EMSCRIPTEN_BINDINGS(xboy) {
    emscripten::class_<WebEmulator>("WebEmulator")
        .constructor<std::string>()
        .function("stepFrame", &WebEmulator::stepFrame)
        .function("setButtonState", &WebEmulator::setButtonState)
        .function("saveState", &WebEmulator::saveState)
        .function("loadState", &WebEmulator::loadState);
        
    // Register exception translator for better error messages
    emscripten::register_optional<std::string>();
}

int main() {
    return 0;
}

#endif
