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
    Weapon,
    Projectile,
    Melee,
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

struct SecretFound {};

struct PlayerFired {
    EntityId shooter_id = 0;
    std::uint8_t weapon = 0;
    float origin_x = 0.f;
    float origin_y = 0.f;
    float aim_x = 0.f;
    float aim_y = 0.f;
    /// For melee weapons: whether the swing connected.
    bool melee_hit = true;
};

struct EntityDamaged {
    EntityId target_id = 0;
    EntityId source_id = 0;
    int amount = 0;
    DamageSource source = DamageSource::Weapon;
    int health_remaining = 0;
};

struct EntityDied {
    EntityId entity_id = 0;
    DamageSource source = DamageSource::Weapon;
};

struct MonsterDied {
    EntityId entity_id = 0;
    std::uint8_t monster_type = 0;
};

enum class ExplosionKind : std::uint8_t {
    Rocket = 0,
    Plasma = 1,
    Bfg = 2,
    ImpFire = 3,
    CacoFire = 4,
    BaronFire = 5,
    BspFire = 6,
    SkelFire = 7,
    MancubFire = 8,
};

struct WorldExplosion {
    ExplosionKind kind = ExplosionKind::Rocket;
    float x = 0.f;
    float y = 0.f;
};

struct WorldSmokePuff {
    float x = 0.f;
    float y = 0.f;
};

struct WorldBubblePuff {
    float x = 0.f;
    float y = 0.f;
};

struct ItemPickedUp {
    std::uint8_t item_type = 0;
};

struct ItemRespawned {
    float x = 0.f;
    float y = 0.f;
};

struct PlayerTeleported {
    bool blocked = false;
};

enum class DoorSound : std::uint8_t {
    None = 0,
    Open,
    Close,
};

struct WorldDoorChanged {
    float x = 0.f;
    float y = 0.f;
    DoorSound sound = DoorSound::None;
};

struct WorldSwitchActivated {
    float x = 0.f;
    float y = 0.f;
};

struct MonsterAlerted {
    EntityId entity_id = 0;
    std::uint8_t monster_type = 0;
    float x = 0.f;
    float y = 0.f;
};

struct MonsterFired {
    EntityId shooter_id = 0;
    std::uint8_t monster_type = 0;
    float x = 0.f;
    float y = 0.f;
};

} // namespace d2df::events
