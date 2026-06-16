#pragma once

#include <d2df/core/types.hpp>

namespace d2df::events {

struct AppStarted {};

struct AppShutdown {};

struct FrameTick {
    float delta_seconds = 0.f;
};

} // namespace d2df::events
