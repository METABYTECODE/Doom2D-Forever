#include <d2df/engine/map/entity_catalog.hpp>

#include <algorithm>
#include <cctype>
#include <string>

#include <d2df/map/monster_types.hpp>

namespace d2df::engine::map {
namespace {

std::string lower_ascii(std::string_view text) {
    std::string out(text);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

::d2df::map::MonsterType monster_type_from_entity_id(std::string_view type_id) {
    const auto lowered = lower_ascii(type_id);
    if (lowered == "demon") {
        return ::d2df::map::MonsterType::Demon;
    }
    if (lowered == "imp") {
        return ::d2df::map::MonsterType::Imp;
    }
    if (lowered == "zomby" || lowered == "zombie") {
        return ::d2df::map::MonsterType::Zomby;
    }
    if (lowered == "serg" || lowered == "sergeant") {
        return ::d2df::map::MonsterType::Serg;
    }
    if (lowered == "cyber") {
        return ::d2df::map::MonsterType::Cyber;
    }
    if (lowered == "cgun") {
        return ::d2df::map::MonsterType::Cgun;
    }
    if (lowered == "baron") {
        return ::d2df::map::MonsterType::Baron;
    }
    if (lowered == "knight") {
        return ::d2df::map::MonsterType::Knight;
    }
    if (lowered == "caco") {
        return ::d2df::map::MonsterType::Caco;
    }
    if (lowered == "soul") {
        return ::d2df::map::MonsterType::Soul;
    }
    if (lowered == "pain") {
        return ::d2df::map::MonsterType::Pain;
    }
    if (lowered == "spider") {
        return ::d2df::map::MonsterType::Spider;
    }
    if (lowered == "bsp") {
        return ::d2df::map::MonsterType::Bsp;
    }
    if (lowered == "mancub") {
        return ::d2df::map::MonsterType::Mancub;
    }
    if (lowered == "skel") {
        return ::d2df::map::MonsterType::Skel;
    }
    if (lowered == "vile") {
        return ::d2df::map::MonsterType::Vile;
    }
    if (lowered == "fish") {
        return ::d2df::map::MonsterType::Fish;
    }
    if (lowered == "barrel") {
        return ::d2df::map::MonsterType::Barrel;
    }
    if (lowered == "robo") {
        return ::d2df::map::MonsterType::Robo;
    }
    if (lowered == "man") {
        return ::d2df::map::MonsterType::Man;
    }

    return ::d2df::map::monster_type_from_name(type_id);
}

} // namespace

std::optional<EntityDimensions> entity_dimensions(std::string_view type_id) {
    const auto monster_type = monster_type_from_entity_id(type_id);
    if (monster_type == ::d2df::map::MonsterType::None) {
        return std::nullopt;
    }

    const auto stats = ::d2df::map::monster_stats(monster_type);
    return EntityDimensions{stats.width, stats.height};
}

} // namespace d2df::engine::map
