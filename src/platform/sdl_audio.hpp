#pragma once

#include <SDL2/SDL.h>
#include <cstdint>
#include <vector>

namespace platform {

// SDL2 audio output using the push-based SDL_QueueAudio API.
// Buffers stereo float samples and flushes in batches to minimize overhead.
class SdlAudio {
public:
  static constexpr int SAMPLE_RATE = 44100;

  SdlAudio();
  ~SdlAudio();

  SdlAudio(const SdlAudio &) = delete;
  SdlAudio &operator=(const SdlAudio &) = delete;

  // Push one stereo sample (left, right) into the buffer.
  // Automatically flushes to SDL when the buffer is full.
  void pushSample(float left, float right);

  // Returns the number of bytes queued in SDL's audio buffer.
  // Useful for throttling to prevent audio queue from growing unbounded.
  uint32_t getQueuedBytes() const;

private:
  SDL_AudioDeviceID device_ = 0;

  // Internal buffer: interleaved L,R,L,R,...
  static constexpr size_t BUFFER_SIZE = 1024; // samples (512 stereo pairs)
  std::vector<float> buffer_;

  void flush();
};

} // namespace platform
