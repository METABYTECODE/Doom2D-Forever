#pragma once

#include <d2df/core/types.hpp>

namespace d2df::events {

struct AppStarted {};

struct AppShutdown {};

struct FrameTick {
    float delta_seconds = 0.f;
};

enum class DamageSource : std::uint8_t {
    Acid,
    Trap,
    Water,
};

struct PlayerLanded {
    float x = 0.f;
    float y = 0.f;
};

struct PlayerEnteredLiquid {
    bool water = false;
    bool acid = false;
};

struct PlayerExitedLiquid {};

struct PlayerDamaged {
    int amount = 0;
    DamageSource source = DamageSource::Acid;
    int health_remaining = 0;
};

struct PlayerDied {
    DamageSource source = DamageSource::Trap;
};

struct MapExitRequested {};

} // namespace d2df::events
