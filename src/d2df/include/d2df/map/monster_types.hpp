#pragma once

#include <cstdint>
#include <string_view>

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

[[nodiscard]] MonsterType monster_type_from_name(std::string_view name);
[[nodiscard]] MonsterDimensions monster_stats(MonsterType type);

} // namespace d2df::map
