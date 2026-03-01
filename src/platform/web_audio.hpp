#pragma once

#include <cstdint>
#include <SDL.h>

namespace platform {

class WebAudio {
public:
    WebAudio();
    ~WebAudio();

    WebAudio(const WebAudio &) = delete;
    WebAudio &operator=(const WebAudio &) = delete;

    void pushSample(float left, float right);
    uint32_t getQueuedBytes() const;

    static constexpr int SAMPLE_RATE = 44100;

private:
    SDL_AudioDeviceID audio_device_;
    SDL_AudioSpec audio_spec_;
};

} // namespace platform