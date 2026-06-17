#pragma once

#include <d2df/core/types.hpp>
#include <d2df/sim/player_state.hpp>
#include <d2df/sim/shootable_target.hpp>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace d2df::ecs {
class GameWorld;
} // namespace d2df::ecs

namespace d2df::sim {

class ItemSystem;
class TriggerSystem;

constexpr int kGameSaveVersion = 1;

struct PlayerCombatSnapshot {
    WeaponId current_weapon = WeaponId::Pistol;
    bool owned[kWeaponSlotCount] = {};
    int ammo[kAmmoTypeCount] = {};
    int max_ammo[kAmmoTypeCount] = {};
    int reloading[kWeaponSlotCount] = {};
    bool facing_left = false;
    int bfg_charge_ticks = -1;
    float bfg_muzzle_x = 0.0f;
    float bfg_muzzle_y = 0.0f;
    float bfg_aim_x = 0.0f;
    float bfg_aim_y = 0.0f;
};

struct PlayerStateSnapshot {
    float x = 0.0f;
    float y = 0.0f;
    float prev_x = 0.0f;
    float prev_y = 0.0f;
    int vel_x = 0;
    int vel_y = 0;
    int accel_x = 0;
    int accel_y = 0;
    int tick = 0;
    int health = PlayerState::kMaxHealth;
    int armor = 0;
    bool key_red = false;
    bool key_green = false;
    bool key_blue = false;
    bool has_backpack = false;
    int powerup_suit_until = 0;
    int powerup_invul_until = 0;
    int powerup_invis_until = 0;
    int berserk_until = 0;
    int jet_fuel = 0;
    int air = PlayerState::kAirMax;
    bool jetpack_active = false;
    bool can_jetpack = false;
    bool on_ground = false;
    bool in_water = false;
    bool in_acid = false;
    bool on_lift = false;
    int run_direction = 0;
    int pain_ticks = 0;
    int punch_ticks = 0;
    bool punch_aim_up = false;
    bool punch_aim_down = false;
    PlayerDeathPhase death_phase = PlayerDeathPhase::None;
    int death_started_tick = 0;
    int death_health = 0;
    bool corpse_resolved = false;
    int respawn_ticks_remaining = 0;
    PlayerCombatSnapshot combat;
};

struct MonsterSnapshot {
    EntityId id = 0;
    std::uint8_t monster_type = 0;
    float x = 0.0f;
    float y = 0.0f;
    float prev_x = 0.0f;
    float prev_y = 0.0f;
    float width = 34.0f;
    float height = 52.0f;
    int vel_x = 0;
    int vel_y = 0;
    int health = 100;
    int max_health = 100;
    int melee_cooldown = 0;
    int shoot_cooldown = 0;
    int anim_tick = 0;
    int revive_ticks = 0;
    bool facing_left = false;
    bool aggro_player = false;
    bool is_awake = false;
    bool in_water = false;
    bool death_handled = false;
    MonsterLifeState life_state = MonsterLifeState::Alive;
};

struct WorldItemSnapshot {
    std::uint8_t type = 0;
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

struct PanelSnapshot {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t width = 0;
    std::int32_t height = 0;
    std::uint16_t texture = 0;
    std::uint16_t type = 0;
    std::uint8_t alpha = 0;
    std::uint8_t flags = 0;
    bool enabled = true;
};

struct TriggerRuntimeSnapshot {
    bool enabled = true;
    int timeout = 0;
};

struct PanelTimerSnapshot {
    std::int32_t panel_index = -1;
    int ticks_remaining = 0;
};

struct ExpanderSnapshot {
    int press_count = 0;
    int press_time = -1;
};

struct GameSaveDocument {
    int version = kGameSaveVersion;
    std::string map_path;
    std::string map_id;
    PlayerStateSnapshot player;
    std::vector<MonsterSnapshot> monsters;
    std::vector<WorldItemSnapshot> items;
    std::vector<PanelSnapshot> panels;
    std::vector<TriggerRuntimeSnapshot> triggers;
    std::vector<PanelTimerSnapshot> trap_timers;
    std::vector<PanelTimerSnapshot> door5_timers;
    std::vector<ExpanderSnapshot> expanders;
    std::vector<bool> nomonster_boot_done;
};

void capture_player_state(const PlayerState& player, PlayerStateSnapshot& out);
void restore_player_state(PlayerState& player, const PlayerStateSnapshot& in);

void capture_world_save(const ecs::GameWorld& world, const TriggerSystem& triggers,
                        const std::string& map_rel_path, const std::string& map_id,
                        GameSaveDocument& out);
void restore_world_save(ecs::GameWorld& world, TriggerSystem& triggers,
                        const GameSaveDocument& in);

[[nodiscard]] bool write_game_save(const std::filesystem::path& path,
                                   const GameSaveDocument& doc, std::string& error);
[[nodiscard]] bool read_game_save(const std::filesystem::path& path, GameSaveDocument& doc,
                                  std::string& error);

} // namespace d2df::sim
