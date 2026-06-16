#include <d2df/map/monster_types.hpp>

namespace d2df::map {
namespace {

MonsterDimensions make_dims(float width, float height, int health) {
    return {width, height, health};
}

} // namespace

MonsterType monster_type_from_name(std::string_view name) {
    if (name == "MONSTER_DEMON") {
        return MonsterType::Demon;
    }
    if (name == "MONSTER_IMP") {
        return MonsterType::Imp;
    }
    if (name == "MONSTER_ZOMBY") {
        return MonsterType::Zomby;
    }
    if (name == "MONSTER_SERG") {
        return MonsterType::Serg;
    }
    if (name == "MONSTER_CYBER") {
        return MonsterType::Cyber;
    }
    if (name == "MONSTER_CGUN") {
        return MonsterType::Cgun;
    }
    if (name == "MONSTER_BARON") {
        return MonsterType::Baron;
    }
    if (name == "MONSTER_KNIGHT") {
        return MonsterType::Knight;
    }
    if (name == "MONSTER_CACO") {
        return MonsterType::Caco;
    }
    if (name == "MONSTER_SOUL") {
        return MonsterType::Soul;
    }
    if (name == "MONSTER_PAIN") {
        return MonsterType::Pain;
    }
    if (name == "MONSTER_SPIDER") {
        return MonsterType::Spider;
    }
    if (name == "MONSTER_BSP") {
        return MonsterType::Bsp;
    }
    if (name == "MONSTER_MANCUB") {
        return MonsterType::Mancub;
    }
    if (name == "MONSTER_SKEL") {
        return MonsterType::Skel;
    }
    if (name == "MONSTER_VILE") {
        return MonsterType::Vile;
    }
    if (name == "MONSTER_FISH") {
        return MonsterType::Fish;
    }
    if (name == "MONSTER_BARREL") {
        return MonsterType::Barrel;
    }
    if (name == "MONSTER_ROBO") {
        return MonsterType::Robo;
    }
    if (name == "MONSTER_MAN") {
        return MonsterType::Man;
    }
    return MonsterType::None;
}

MonsterDimensions monster_stats(MonsterType type) {
    switch (type) {
    case MonsterType::Demon:
        return make_dims(50.0f, 52.0f, 60);
    case MonsterType::Imp:
        return make_dims(34.0f, 50.0f, 25);
    case MonsterType::Zomby:
        return make_dims(34.0f, 52.0f, 15);
    case MonsterType::Serg:
        return make_dims(34.0f, 52.0f, 20);
    case MonsterType::Cyber:
        return make_dims(80.0f, 110.0f, 500);
    case MonsterType::Cgun:
        return make_dims(34.0f, 56.0f, 60);
    case MonsterType::Baron:
        return make_dims(50.0f, 64.0f, 150);
    case MonsterType::Knight:
        return make_dims(50.0f, 64.0f, 75);
    case MonsterType::Caco:
        return make_dims(60.0f, 56.0f, 100);
    case MonsterType::Soul:
        return make_dims(32.0f, 36.0f, 60);
    case MonsterType::Pain:
        return make_dims(60.0f, 56.0f, 100);
    case MonsterType::Spider:
        return make_dims(210.0f, 100.0f, 500);
    case MonsterType::Bsp:
        return make_dims(100.0f, 42.0f, 150);
    case MonsterType::Mancub:
        return make_dims(72.0f, 60.0f, 200);
    case MonsterType::Skel:
        return make_dims(68.0f, 72.0f, 200);
    case MonsterType::Vile:
        return make_dims(68.0f, 72.0f, 150);
    case MonsterType::Fish:
        return make_dims(20.0f, 10.0f, 35);
    case MonsterType::Barrel:
        return make_dims(24.0f, 36.0f, 20);
    case MonsterType::Robo:
        return make_dims(68.0f, 76.0f, 20);
    case MonsterType::Man:
        return make_dims(34.0f, 52.0f, 400);
    default:
        return make_dims(34.0f, 52.0f, 100);
    }
}

} // namespace d2df::map
