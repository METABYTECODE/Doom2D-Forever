#include <d2df/map/monster_types.hpp>

namespace d2df::map {
namespace {

MonsterDimensions make_dims(float width, float height, int health) {
    return {width, height, health};
}

MonsterSprite make_sprite(const char* texture_id, int frame_width, int frame_height, int frame_count,
                          int anim_period, const char* texture_id_left = nullptr) {
    return {texture_id, texture_id_left, frame_width, frame_height, frame_count, anim_period};
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

bool monster_is_flying(MonsterType type) {
    switch (type) {
    case MonsterType::Soul:
    case MonsterType::Pain:
    case MonsterType::Caco:
        return true;
    default:
        return false;
    }
}

bool monster_vanishes_on_death(MonsterType type) {
    switch (type) {
    case MonsterType::Soul:
    case MonsterType::Pain:
    case MonsterType::Barrel:
        return true;
    default:
        return false;
    }
}

bool monster_can_shoot(MonsterType type) {
    switch (type) {
    case MonsterType::Demon:
    case MonsterType::Fish:
    case MonsterType::Barrel:
    case MonsterType::Pain:
    case MonsterType::Vile:
        return false;
    default:
        return type != MonsterType::None;
    }
}

bool monster_can_be_revived(MonsterType type) {
    switch (type) {
    case MonsterType::Soul:
    case MonsterType::Pain:
    case MonsterType::Cyber:
    case MonsterType::Spider:
    case MonsterType::Vile:
    case MonsterType::Barrel:
    case MonsterType::Robo:
        return false;
    default:
        return type != MonsterType::None;
    }
}

int monster_shoot_cooldown_ticks(MonsterType type) {
    switch (type) {
    case MonsterType::Cyber:
    case MonsterType::Spider:
        return 54;
    case MonsterType::Serg:
    case MonsterType::Cgun:
        return 36;
    default:
        return 45;
    }
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

MonsterSprite monster_sprite(MonsterType type, bool /*facing_left*/) {
    switch (type) {
    case MonsterType::Zomby:
        return make_sprite("tex.monster.zomby_go", 64, 64, 4, 3);
    case MonsterType::Imp:
        return make_sprite("tex.monster.imp_go", 64, 64, 4, 3);
    case MonsterType::Demon:
        return make_sprite("tex.monster.demon_go", 64, 64, 4, 3);
    case MonsterType::Serg:
        return make_sprite("tex.monster.serg_go", 64, 64, 4, 3);
    case MonsterType::Soul:
        return make_sprite("tex.monster.soul_go", 64, 64, 4, 3);
    case MonsterType::Pain:
        return make_sprite("tex.monster.pain_go", 128, 128, 4, 3);
    case MonsterType::Caco:
        return make_sprite("tex.monster.caco_go", 128, 128, 4, 3);
    case MonsterType::Cgun:
        return make_sprite("tex.monster.cgun_go", 64, 64, 4, 3, "tex.monster.cgun_go_l");
    case MonsterType::Baron:
        return make_sprite("tex.monster.baron_go", 128, 128, 4, 3, "tex.monster.baron_go");
    case MonsterType::Knight:
        return make_sprite("tex.monster.knight_go", 128, 128, 4, 3);
    case MonsterType::Vile:
        return make_sprite("tex.monster.vile_go", 128, 128, 6, 3);
    case MonsterType::Barrel:
        return make_sprite("tex.monster.barrel_sleep", 64, 64, 3, 5);
    case MonsterType::Fish:
        return make_sprite("tex.monster.fish_go", 32, 32, 4, 3);
    case MonsterType::Cyber:
        return make_sprite("tex.monster.cyber_go", 128, 128, 4, 4);
    case MonsterType::Spider:
        return make_sprite("tex.monster.spider_go", 256, 128, 4, 3);
    case MonsterType::Bsp:
        return make_sprite("tex.monster.bsp_go", 128, 64, 4, 3);
    case MonsterType::Mancub:
        return make_sprite("tex.monster.mancub_go", 128, 128, 4, 3);
    case MonsterType::Skel:
        return make_sprite("tex.monster.skel_go", 128, 128, 4, 3);
    default:
        return {};
    }
}

MonsterSprite monster_corpse_sprite(MonsterType type, bool facing_left) {
    (void)facing_left;
    switch (type) {
    case MonsterType::Zomby:
        return make_sprite("tex.monster.zomby_die", 64, 64, 5, 0);
    case MonsterType::Imp:
        return make_sprite("tex.monster.imp_die", 64, 64, 5, 0);
    case MonsterType::Demon:
        return make_sprite("tex.monster.demon_die_2", 64, 64, 5, 0);
    case MonsterType::Serg:
        return make_sprite("tex.monster.serg_die", 64, 64, 5, 0);
    case MonsterType::Cgun:
        return make_sprite("tex.monster.cgun_die", 64, 64, 5, 0);
    case MonsterType::Baron:
        return make_sprite("tex.monster.baron_die_2", 128, 128, 7, 0);
    case MonsterType::Knight:
        return make_sprite("tex.monster.knight_die_2", 128, 128, 7, 0);
    case MonsterType::Caco:
        return make_sprite("tex.monster.caco_die_2", 128, 128, 7, 0);
    case MonsterType::Vile:
        return make_sprite("tex.monster.vile_die_2", 128, 128, 9, 0);
    case MonsterType::Fish:
        return make_sprite("tex.monster.fish_die", 32, 32, 5, 0);
    default:
        return monster_sprite(type, facing_left);
    }
}

} // namespace d2df::map
