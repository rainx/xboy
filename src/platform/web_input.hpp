#pragma once

#include "input/joypad.hpp"
#include <memory>
#include <SDL.h>

namespace platform {

class WebInput {
public:
    explicit WebInput(std::shared_ptr<input::Joypad> joypad);
    ~WebInput();

    WebInput(const WebInput &) = delete;
    WebInput &operator=(const WebInput &) = delete;

    bool poll();

private:
    std::shared_ptr<input::Joypad> joypad_;
};

} // namespace platform