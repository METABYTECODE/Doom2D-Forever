#pragma once

#include <cstdint>
#include <string_view>
#include <d2df/core/types.hpp>
#include <d2df/map/item_types.hpp>

namespace d2df::map {

enum class MonsterType : std::uint8_t {
    None = 0,
    Demon = 1,
    Imp = 2,
    Zomby = 3,
    Serg = 4,
    Cyber = 5,
    Cgun = 6,
    Baron = 7,
    Knight = 8,
    Caco = 9,
    Soul = 10,
    Pain = 11,
    Spider = 12,
    Bsp = 13,
    Mancub = 14,
    Skel = 15,
    Vile = 16,
    Fish = 17,
    Barrel = 18,
    Robo = 19,
    Man = 20,
};

struct MonsterDimensions {
    float width = 34.0f;
    float height = 52.0f;
    int health = 100;
};

struct MonsterHitbox {
    /// Legacy MONSTERTABLE rect origin inside the sprite sheet anchor.
    float origin_offset_x = 0.0f;
    float origin_offset_y = 0.0f;
};

struct MonsterSprite {
    const char* texture_id = nullptr;
    /// When set, used instead of horizontal flip for facing_left.
    const char* texture_id_left = nullptr;
    int frame_width = 0;
    int frame_height = 0;
    int frame_count = 1;
    int anim_period = 0;
    float draw_offset_x = 0.0f;
    float draw_offset_y = 0.0f;
    float draw_offset_left_x = 0.0f;
    float draw_offset_left_y = 0.0f;
    bool has_draw_offset_left = false;
};

struct MonsterSpritePlacement {
    float x = 0.0f;
    float y = 0.0f;
    bool flip_horizontal = false;
};

[[nodiscard]] MonsterType monster_type_from_name(std::string_view name);
[[nodiscard]] MonsterDimensions monster_stats(MonsterType type);
[[nodiscard]] MonsterHitbox monster_hitbox(MonsterType type);
[[nodiscard]] MonsterSprite monster_sprite(MonsterType type, bool facing_left);
[[nodiscard]] MonsterSprite monster_corpse_sprite(MonsterType type, bool facing_left);
[[nodiscard]] MonsterSpritePlacement monster_sprite_placement(MonsterType type, float hitbox_x,
                                                              float hitbox_y, float hitbox_width,
                                                              const MonsterSprite& sprite,
                                                              bool facing_left);
[[nodiscard]] bool monster_is_flying(MonsterType type);
[[nodiscard]] bool monster_vanishes_on_death(MonsterType type);
[[nodiscard]] bool monster_can_shoot(MonsterType type);
[[nodiscard]] bool monster_can_be_revived(MonsterType type);
[[nodiscard]] int monster_shoot_cooldown_ticks(MonsterType type);
[[nodiscard]] const char* monster_die_sfx(MonsterType type);
[[nodiscard]] const char* monster_alert_sfx(MonsterType type, EntityId entity_id);
[[nodiscard]] ItemType monster_death_loot(MonsterType type);

} // namespace d2df::map
