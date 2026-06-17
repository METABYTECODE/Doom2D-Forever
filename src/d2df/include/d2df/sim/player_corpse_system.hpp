#pragma once

#include <cstdint>
#include <vector>

namespace d2df::sim {

class MapCollision;
struct PlayerState;

struct PlayerCorpse {
    float x = 0.0f;
    float y = 0.0f;
    float prev_x = 0.0f;
    float prev_y = 0.0f;
    int vel_x = 0;
    int vel_y = 0;
    bool facing_left = false;
    bool mess = false;
    bool active = true;
    int tick = 0;
};

struct PlayerGib {
    float x = 0.0f;
    float y = 0.0f;
    float prev_x = 0.0f;
    float prev_y = 0.0f;
    int vel_x = 0;
    int vel_y = 0;
    int src_frame = 0;
    int rotation_deg = 0;
    bool active = true;
    int tick = 0;
};

constexpr int kPlayerDeathGibHealthThreshold = -50;
constexpr int kPlayerDeathMessHealthThreshold = -20;
constexpr int kPlayerCorpseFalloutBelowMap = 128;
constexpr int kPlayerGibCount = 6;
constexpr int kPlayerGibFrameSize = 32;

class PlayerCorpseSystem {
public:
    void clear();

    void spawn_from_death(const PlayerState& player, int map_height);

    void tick(const MapCollision& collision, int map_height);

    [[nodiscard]] const std::vector<PlayerCorpse>& corpses() const { return corpses_; }
    [[nodiscard]] const std::vector<PlayerGib>& gibs() const { return gibs_; }

private:
    void spawn_corpse(const PlayerState& player);
    void spawn_gibs(float center_x, float center_y, int base_vel_x, int base_vel_y);

    void tick_corpse(PlayerCorpse& corpse, const MapCollision& collision, int map_height);
    void tick_gib(PlayerGib& gib, const MapCollision& collision, int map_height);

    std::vector<PlayerCorpse> corpses_;
    std::vector<PlayerGib> gibs_;
};

} // namespace d2df::sim
