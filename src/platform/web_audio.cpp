#include "platform/web_audio.hpp"
#include <stdexcept>
#include <SDL.h>
#include <vector>
#include <mutex>

namespace platform {

WebAudio::WebAudio() {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        throw std::runtime_error(std::string("SDL_Init AUDIO failed: ") + SDL_GetError());
    }

    SDL_AudioSpec desired_spec;
    SDL_zero(desired_spec);
    desired_spec.freq = SAMPLE_RATE;
    desired_spec.format = AUDIO_F32;
    desired_spec.channels = 2;
    desired_spec.samples = 1024;
    desired_spec.callback = nullptr; // We'll use SDL_QueueAudio

    audio_device_ = SDL_OpenAudioDevice(nullptr, 0, &desired_spec, &audio_spec_, 0);
    if (audio_device_ == 0) {
        throw std::runtime_error(std::string("SDL_OpenAudioDevice failed: ") + SDL_GetError());
    }

    // Start audio playback
    SDL_PauseAudioDevice(audio_device_, 0);
}

WebAudio::~WebAudio() {
    if (audio_device_) {
        SDL_CloseAudioDevice(audio_device_);
    }
}

void WebAudio::pushSample(float left, float right) {
    float samples[2] = {left, right};
    SDL_QueueAudio(audio_device_, samples, sizeof(samples));
}

uint32_t WebAudio::getQueuedBytes() const {
    return SDL_GetQueuedAudioSize(audio_device_);
}

} // namespace platform