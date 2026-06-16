#include <d2df/map/monster_types.hpp>

#include <d2df/core/types.hpp>

namespace d2df::map {
namespace {

MonsterDimensions make_dims(float width, float height, int health) {
    return {width, height, health};
}

MonsterSprite make_sprite(const char* texture_id, int frame_width, int frame_height,
                          int frame_count, int anim_period, float draw_offset_x,
                          float draw_offset_y, const char* texture_id_left = nullptr,
                          float draw_offset_left_x = 0.0f, float draw_offset_left_y = 0.0f,
                          bool has_draw_offset_left = false) {
    MonsterSprite sprite{texture_id,
                         texture_id_left,
                         frame_width,
                         frame_height,
                         frame_count,
                         anim_period,
                         draw_offset_x,
                         draw_offset_y,
                         draw_offset_left_x,
                         draw_offset_left_y,
                         has_draw_offset_left};
    return sprite;
}

MonsterHitbox make_hitbox(float origin_offset_x, float origin_offset_y) {
    return {origin_offset_x, origin_offset_y};
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
        return make_sprite("tex.monster.zomby_go", 64, 64, 4, 3, 1.0f, -4.0f);
    case MonsterType::Imp:
        return make_sprite("tex.monster.imp_go", 64, 64, 4, 3, 8.0f, -4.0f);
    case MonsterType::Demon:
        return make_sprite("tex.monster.demon_go", 64, 64, 4, 3, 1.0f, 4.0f);
    case MonsterType::Serg:
        return make_sprite("tex.monster.serg_go", 64, 64, 4, 3, 0.0f, -4.0f);
    case MonsterType::Soul:
        return make_sprite("tex.monster.soul_go", 64, 64, 2, 3, 1.0f, -10.0f);
    case MonsterType::Pain:
        return make_sprite("tex.monster.pain_go", 128, 128, 4, 3, -1.0f, -3.0f);
    case MonsterType::Caco:
        return make_sprite("tex.monster.caco_go", 128, 128, 1, 3, 0.0f, -4.0f);
    case MonsterType::Cgun:
        return make_sprite("tex.monster.cgun_go", 64, 64, 4, 3, -1.0f, -2.0f,
                           "tex.monster.cgun_go_l");
    case MonsterType::Baron:
        return make_sprite("tex.monster.baron_go", 128, 128, 4, 3, 2.0f, 0.0f,
                           "tex.monster.baron_go", 2.0f, 0.0f, true);
    case MonsterType::Knight:
        return make_sprite("tex.monster.knight_go", 128, 128, 4, 3, 2.0f, 0.0f,
                           nullptr, 2.0f, 0.0f, true);
    case MonsterType::Vile:
        return make_sprite("tex.monster.vile_go", 128, 128, 6, 3, 5.0f, -21.0f,
                           nullptr, 5.0f, -21.0f, true);
    case MonsterType::Barrel:
        return make_sprite("tex.monster.barrel_sleep", 64, 64, 3, 5, 0.0f, -15.0f);
    case MonsterType::Fish:
        return make_sprite("tex.monster.fish_go", 32, 32, 4, 2, -1.0f, 0.0f);
    case MonsterType::Cyber:
        return make_sprite("tex.monster.cyber_go", 128, 128, 4, 4, 2.0f, -6.0f,
                           "tex.monster.cyber_go_l", 3.0f, -3.0f, true);
    case MonsterType::Spider:
        return make_sprite("tex.monster.spider_go", 256, 128, 6, 3, -4.0f, -4.0f);
    case MonsterType::Bsp:
        return make_sprite("tex.monster.bsp_go", 128, 64, 6, 3, 0.0f, -1.0f);
    case MonsterType::Mancub:
        return make_sprite("tex.monster.mancub_go", 128, 128, 6, 3, -2.0f, -7.0f);
    case MonsterType::Skel:
        return make_sprite("tex.monster.skel_go", 128, 128, 6, 3, -1.0f, 4.0f,
                           nullptr, -1.0f, 4.0f, true);
    default:
        return {};
    }
}

MonsterSprite monster_sleep_sprite(MonsterType type, bool /*facing_left*/) {
    switch (type) {
    case MonsterType::Zomby:
        return make_sprite("tex.monster.zomby_sleep", 64, 64, 1, 0, 1.0f, -4.0f);
    case MonsterType::Imp:
        return make_sprite("tex.monster.imp_sleep", 64, 64, 1, 0, 8.0f, -4.0f);
    case MonsterType::Demon:
        return make_sprite("tex.monster.demon_sleep", 64, 64, 1, 0, 1.0f, 4.0f);
    case MonsterType::Serg:
        return make_sprite("tex.monster.serg_sleep", 64, 64, 1, 0, 0.0f, -4.0f);
    case MonsterType::Soul:
        return make_sprite("tex.monster.soul_sleep", 64, 64, 1, 0, 1.0f, -10.0f);
    case MonsterType::Pain:
        return make_sprite("tex.monster.pain_sleep", 128, 128, 1, 0, -1.0f, -3.0f);
    case MonsterType::Caco:
        return make_sprite("tex.monster.caco_sleep", 128, 128, 1, 0, 0.0f, -4.0f);
    case MonsterType::Cgun:
        return make_sprite("tex.monster.cgun_sleep", 64, 64, 1, 0, -1.0f, -2.0f,
                           "tex.monster.cgun_sleep_l");
    case MonsterType::Baron:
        return make_sprite("tex.monster.baron_sleep", 128, 128, 1, 0, 2.0f, 0.0f,
                           "tex.monster.baron_sleep_l", 2.0f, 0.0f, true);
    case MonsterType::Knight:
        return make_sprite("tex.monster.knight_sleep", 128, 128, 1, 0, 2.0f, 0.0f,
                           "tex.monster.knight_sleep_l", 2.0f, 0.0f, true);
    case MonsterType::Vile:
        return make_sprite("tex.monster.vile_sleep", 128, 128, 1, 0, 5.0f, -21.0f,
                           "tex.monster.vile_sleep_l", 5.0f, -21.0f, true);
    case MonsterType::Barrel:
        return make_sprite("tex.monster.barrel_sleep", 64, 64, 3, 5, 0.0f, -15.0f);
    case MonsterType::Fish:
        return make_sprite("tex.monster.fish_sleep", 32, 32, 1, 0, -1.0f, 0.0f);
    case MonsterType::Cyber:
        return make_sprite("tex.monster.cyber_sleep", 128, 128, 1, 0, 2.0f, -6.0f,
                           "tex.monster.cyber_sleep_l", 3.0f, -3.0f, true);
    case MonsterType::Spider:
        return make_sprite("tex.monster.spider_sleep", 256, 128, 1, 0, -4.0f, -4.0f);
    case MonsterType::Bsp:
        return make_sprite("tex.monster.bsp_sleep", 128, 64, 1, 0, 0.0f, -1.0f);
    case MonsterType::Mancub:
        return make_sprite("tex.monster.mancub_sleep", 128, 128, 1, 0, -2.0f, -7.0f);
    case MonsterType::Skel:
        return make_sprite("tex.monster.skel_sleep", 128, 128, 1, 0, -1.0f, 4.0f,
                           "tex.monster.skel_sleep_l", -1.0f, 4.0f, true);
    case MonsterType::Robo:
        return make_sprite("tex.monster.robo_sleep", 128, 128, 1, 0, 0.0f, 0.0f);
    case MonsterType::Man:
        return make_sprite("tex.monster.man_sleep", 64, 64, 1, 0, 0.0f, -4.0f);
    default:
        return monster_sprite(type, false);
    }
}

MonsterSprite monster_corpse_sprite(MonsterType type, bool /*facing_left*/) {
    switch (type) {
    case MonsterType::Zomby:
        return make_sprite("tex.monster.zomby_die", 64, 64, 6, 0, 3.0f, -1.0f);
    case MonsterType::Imp:
        return make_sprite("tex.monster.imp_die", 64, 64, 5, 0, -2.0f, -1.0f);
    case MonsterType::Demon:
        return make_sprite("tex.monster.demon_die_2", 64, 64, 6, 0, 0.0f, 4.0f);
    case MonsterType::Serg:
        return make_sprite("tex.monster.serg_die", 64, 64, 5, 0, -3.0f, -1.0f);
    case MonsterType::Cgun:
        return make_sprite("tex.monster.cgun_die", 64, 64, 7, 0, -2.0f, 0.0f);
    case MonsterType::Baron:
        return make_sprite("tex.monster.baron_die_2", 128, 128, 7, 0, -1.0f, -1.0f);
    case MonsterType::Knight:
        return make_sprite("tex.monster.knight_die_2", 128, 128, 7, 0, -1.0f, -1.0f);
    case MonsterType::Caco:
        return make_sprite("tex.monster.caco_die_2", 128, 128, 7, 0, 0.0f, -5.0f);
    case MonsterType::Vile:
        return make_sprite("tex.monster.vile_die_2", 128, 128, 9, 0, 1.0f, -21.0f);
    case MonsterType::Fish:
        return make_sprite("tex.monster.fish_die", 32, 32, 1, 0, -2.0f, -1.0f);
    default:
        return monster_sprite(type, false);
    }
}

MonsterHitbox monster_hitbox(MonsterType type) {
    switch (type) {
    case MonsterType::Demon:
        return make_hitbox(7.0f, 8.0f);
    case MonsterType::Imp:
        return make_hitbox(15.0f, 10.0f);
    case MonsterType::Zomby:
        return make_hitbox(15.0f, 8.0f);
    case MonsterType::Serg:
        return make_hitbox(15.0f, 8.0f);
    case MonsterType::Cyber:
        return make_hitbox(24.0f, 9.0f);
    case MonsterType::Cgun:
        return make_hitbox(15.0f, 4.0f);
    case MonsterType::Baron:
        return make_hitbox(39.0f, 32.0f);
    case MonsterType::Knight:
        return make_hitbox(39.0f, 32.0f);
    case MonsterType::Caco:
        return make_hitbox(34.0f, 36.0f);
    case MonsterType::Soul:
        return make_hitbox(16.0f, 14.0f);
    case MonsterType::Pain:
        return make_hitbox(34.0f, 36.0f);
    case MonsterType::Spider:
        return make_hitbox(23.0f, 14.0f);
    case MonsterType::Bsp:
        return make_hitbox(14.0f, 17.0f);
    case MonsterType::Mancub:
        return make_hitbox(28.0f, 34.0f);
    case MonsterType::Skel:
        return make_hitbox(30.0f, 28.0f);
    case MonsterType::Vile:
        return make_hitbox(30.0f, 28.0f);
    case MonsterType::Fish:
        return make_hitbox(6.0f, 11.0f);
    case MonsterType::Barrel:
        return make_hitbox(20.0f, 13.0f);
    case MonsterType::Robo:
        return make_hitbox(30.0f, 26.0f);
    case MonsterType::Man:
        return make_hitbox(15.0f, 6.0f);
    default:
        return {};
    }
}

MonsterSpritePlacement monster_sprite_placement(MonsterType type, float hitbox_x,
                                                float hitbox_y, float hitbox_width,
                                                const MonsterSprite& sprite, bool facing_left) {
    const auto hitbox = monster_hitbox(type);
    const float object_x = hitbox_x - hitbox.origin_offset_x;
    const float object_y = hitbox_y - hitbox.origin_offset_y;

    float draw_x = sprite.draw_offset_x;
    float draw_y = sprite.draw_offset_y;
    bool flip_horizontal = false;

    const bool has_dedicated_left_texture =
        sprite.texture_id_left != nullptr && sprite.texture_id_left != sprite.texture_id;

    if (facing_left && type != MonsterType::Barrel) {
        if (sprite.has_draw_offset_left) {
            draw_x = sprite.draw_offset_left_x;
            draw_y = sprite.draw_offset_left_y;
        }

        if (!has_dedicated_left_texture) {
            const float anchor = (hitbox.origin_offset_x - draw_x) + hitbox_width;
            draw_x = static_cast<float>(sprite.frame_width) - anchor - hitbox.origin_offset_x;
            flip_horizontal = true;
        }
    }

    return {object_x + draw_x, object_y + draw_y, flip_horizontal};
}

const char* monster_die_sfx(MonsterType type) {
    switch (type) {
    case MonsterType::Zomby:
    case MonsterType::Serg:
    case MonsterType::Cgun:
        return "sfx.monster.die_1";
    case MonsterType::Imp:
        return "sfx.monster.imp_die_1";
    case MonsterType::Demon:
        return "sfx.monster.demon_die";
    case MonsterType::Soul:
        return "sfx.monster.soul_die";
    case MonsterType::Pain:
        return "sfx.monster.pain_die";
    case MonsterType::Caco:
        return "sfx.monster.caco_die";
    case MonsterType::Baron:
        return "sfx.monster.baron_die";
    case MonsterType::Knight:
        return "sfx.monster.knight_die";
    case MonsterType::Vile:
        return "sfx.monster.vile_die";
    case MonsterType::Barrel:
        return "sfx.monster.barrel_die";
    case MonsterType::Cyber:
        return "sfx.monster.cyber_die";
    case MonsterType::Spider:
        return "sfx.monster.spider_die";
    case MonsterType::Bsp:
        return "sfx.monster.bsp_die";
    case MonsterType::Mancub:
        return "sfx.monster.mancub_die";
    case MonsterType::Skel:
        return "sfx.monster.skel_die";
    case MonsterType::Fish:
        return nullptr;
    default:
        return "sfx.monster.die_2";
    }
}

const char* monster_alert_sfx(MonsterType type, EntityId entity_id) {
    switch (type) {
    case MonsterType::Imp:
        return (entity_id % 2) == 0 ? "sfx.monster.imp_alert_1" : "sfx.monster.imp_alert_2";
    case MonsterType::Zomby:
    case MonsterType::Serg:
    case MonsterType::Cgun:
        switch (entity_id % 3) {
        case 0:
            return "sfx.monster.alert_1";
        case 1:
            return "sfx.monster.alert_2";
        default:
            return "sfx.monster.alert_3";
        }
    case MonsterType::Man:
        return "sfx.monster.man_alert";
    case MonsterType::Bsp:
        return "sfx.monster.bsp_alert";
    case MonsterType::Vile:
        return "sfx.monster.vile_alert";
    case MonsterType::Baron:
        return "sfx.monster.baron_alert";
    case MonsterType::Caco:
        return "sfx.monster.caco_alert";
    case MonsterType::Cyber:
        return "sfx.monster.cyber_alert";
    case MonsterType::Knight:
        return "sfx.monster.knight_alert";
    case MonsterType::Mancub:
        return "sfx.monster.mancub_alert";
    case MonsterType::Pain:
        return "sfx.monster.pain_alert";
    case MonsterType::Demon:
        return "sfx.monster.demon_alert";
    case MonsterType::Skel:
        return "sfx.monster.skel_alert";
    case MonsterType::Spider:
        return "sfx.monster.spider_alert";
    default:
        return nullptr;
    }
}

const char* monster_attack_sfx(MonsterType type) {
    switch (type) {
    case MonsterType::Zomby:
        return "sfx.world.firepistol";
    case MonsterType::Serg:
        return "sfx.world.fireshotgun";
    case MonsterType::Man:
        return "sfx.world.fireshotgun2";
    case MonsterType::Cgun:
    case MonsterType::Spider:
        return "sfx.world.firecgun";
    case MonsterType::Imp:
    case MonsterType::Caco:
    case MonsterType::Baron:
    case MonsterType::Knight:
    case MonsterType::Mancub:
        return "sfx.world.fireball";
    case MonsterType::Cyber:
        return "sfx.world.firerocket";
    case MonsterType::Skel:
        return "sfx.world.firerev";
    case MonsterType::Bsp:
    case MonsterType::Robo:
        return "sfx.world.fireplasma";
    default:
        return nullptr;
    }
}

ItemType monster_death_loot(MonsterType type) {
    switch (type) {
    case MonsterType::Zomby:
        return ItemType::AmmoBullets;
    case MonsterType::Serg:
        return ItemType::WeaponShotgun1;
    case MonsterType::Cgun:
        return ItemType::WeaponChaingun;
    case MonsterType::Man:
        return ItemType::KeyRed;
    default:
        return ItemType::None;
    }
}

} // namespace d2df::map
