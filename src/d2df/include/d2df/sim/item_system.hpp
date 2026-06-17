#pragma once

#include <d2df/map/item_types.hpp>
#include <d2df/map/map_document.hpp>
#include <d2df/sim/game_rules.hpp>
#include <d2df/sim/game_save.hpp>
#include <d2df/sim/map_collision.hpp>
#include <d2df/sim/player_state.hpp>

#include <vector>

namespace d2df {

class EventBus;

} // namespace d2df

namespace d2df::sim {

constexpr int kGameTicksPerSecond = 36;
constexpr int kDefaultItemRespawnSeconds = 60;
constexpr int kDefaultItemRespawnTicks = kDefaultItemRespawnSeconds * kGameTicksPerSecond;
constexpr int kItemRespawnAnimFrames = 5;
constexpr int kItemRespawnAnimFramePeriod = 4;
constexpr int kItemRespawnAnimTotalTicks = kItemRespawnAnimFrames * kItemRespawnAnimFramePeriod;

struct WorldItem {
    map::ItemType type = map::ItemType::None;
    float x = 0.0f;
    float y = 0.0f;
    float spawn_x = 0.0f;
    float spawn_y = 0.0f;
    float width = 16.0f;
    float height = 16.0f;
    bool active = true;
    bool respawnable = false;
    bool fall = false;
    bool dropped = false;
    int vel_x = 0;
    int vel_y = 0;
    int anim_tick = 0;
    int respawn_anim_tick = 0;
    int respawn_countdown = 0;
};

class ItemSystem {
public:
    void reset(const map::MapDocument& map, const GameRules& rules = {});

    void tick(PlayerState& player, const MapCollision* collision, EventBus* events);

    void spawn_monster_drop(map::ItemType type, float center_x, float center_y, int vel_x,
                            int vel_y);

    void spawn_player_death_loot(const PlayerState& player);

    [[nodiscard]] const std::vector<WorldItem>& items() const { return items_; }
    [[nodiscard]] const GameRules& rules() const { return rules_; }

    void export_save_state(std::vector<WorldItemSnapshot>& out) const;
    void import_save_state(const std::vector<WorldItemSnapshot>& in);

private:
    std::vector<WorldItem> items_;
    GameRules rules_;
};

} // namespace d2df::sim
