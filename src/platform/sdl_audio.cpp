#include "platform/sdl_audio.hpp"
#include <stdexcept>
#include <string>

namespace platform {

SdlAudio::SdlAudio() {
  // Additive init — SDL_INIT_VIDEO may already be active
  if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
    throw std::runtime_error(std::string("SDL_InitSubSystem(AUDIO) failed: ") +
                             SDL_GetError());
  }

  SDL_AudioSpec want{};
  want.freq = SAMPLE_RATE;
  want.format = AUDIO_F32SYS;
  want.channels = 2;
  want.samples = 512;
  want.callback = nullptr; // push-based (SDL_QueueAudio)

  SDL_AudioSpec have{};
  device_ = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
  if (device_ == 0) {
    throw std::runtime_error(std::string("SDL_OpenAudioDevice failed: ") +
                             SDL_GetError());
  }

  buffer_.reserve(BUFFER_SIZE);

  // Unpause — SDL audio devices start paused
  SDL_PauseAudioDevice(device_, 0);
}

SdlAudio::~SdlAudio() {
  if (device_ != 0) {
    SDL_CloseAudioDevice(device_);
  }
  SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void SdlAudio::pushSample(float left, float right) {
  buffer_.push_back(left);
  buffer_.push_back(right);

  if (buffer_.size() >= BUFFER_SIZE) {
    flush();
  }
}

void SdlAudio::flush() {
  if (!buffer_.empty()) {
    SDL_QueueAudio(device_, buffer_.data(),
                   static_cast<uint32_t>(buffer_.size() * sizeof(float)));
    buffer_.clear();
  }
}

uint32_t SdlAudio::getQueuedBytes() const {
  return SDL_GetQueuedAudioSize(device_);
}

} // namespace platform
