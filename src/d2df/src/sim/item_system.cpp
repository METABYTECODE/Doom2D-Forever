#include <d2df/sim/item_system.hpp>

#include <algorithm>

#include <d2df/core/event_bus.hpp>
#include <d2df/core/game_events.hpp>
#include <d2df/sim/combat_common.hpp>
#include <d2df/sim/weapon_types.hpp>

namespace d2df::sim {
namespace {

constexpr int kHealthSoftCap = 100;
constexpr int kHealthLimit = 200;

int z_dec(int value, int amount) {
    if (value > 0) {
        return std::max(0, value - amount);
    }
    if (value < 0) {
        return std::min(0, value + amount);
    }
    return 0;
}

void inc_max(int& value, int amount, int max_value) {
    value = std::min(value + amount, max_value);
}

bool apply_weapon_pickup(PlayerCombat& combat, WeaponId weapon, AmmoType ammo_type, int ammo_grant,
                         bool switch_to_weapon) {
    const auto weapon_index = static_cast<std::size_t>(weapon);
    const auto ammo_index = static_cast<std::size_t>(ammo_type);
    const bool needs_ammo = ammo_grant > 0;
    if (combat.owned[weapon_index] && needs_ammo &&
        combat.ammo[ammo_index] >= combat.max_ammo[ammo_index]) {
        return false;
    }

    combat.owned[weapon_index] = true;
    if (ammo_grant > 0) {
        inc_max(combat.ammo[ammo_index], ammo_grant, combat.max_ammo[ammo_index]);
    }
    if (switch_to_weapon) {
        combat.select_weapon(weapon);
    }
    return true;
}

bool apply_item_effect(map::ItemType type, PlayerState& player) {
    auto& combat = player.combat();

    switch (type) {
    case map::ItemType::MedkitSmall:
        return player.add_health(10, kHealthSoftCap);

    case map::ItemType::MedkitLarge:
        return player.add_health(25, kHealthSoftCap);

    case map::ItemType::MedkitBlack:
        if (player.health() < kHealthSoftCap) {
            player.set_health(kHealthSoftCap);
        }
        player.give_berserk();
        return true;

    case map::ItemType::SphereBlue:
        return player.add_health(100, kHealthLimit);

    case map::ItemType::SphereWhite:
        if (player.health() >= kHealthLimit && player.armor() >= PlayerState::kArmorLimit) {
            return false;
        }
        if (player.health() < kHealthLimit) {
            player.set_health(kHealthLimit);
        }
        if (player.armor() < PlayerState::kArmorLimit) {
            player.set_armor(PlayerState::kArmorLimit);
        }
        return true;

    case map::ItemType::ArmorGreen:
        if (player.armor() >= PlayerState::kArmorSoftCap) {
            return false;
        }
        player.set_armor(PlayerState::kArmorSoftCap);
        return true;

    case map::ItemType::ArmorBlue:
        if (player.armor() >= PlayerState::kArmorLimit) {
            return false;
        }
        player.set_armor(PlayerState::kArmorLimit);
        return true;

    case map::ItemType::Suit:
        player.give_suit();
        return true;

    case map::ItemType::Oxygen:
        if (player.air() >= PlayerState::kAirMax) {
            return false;
        }
        player.refill_oxygen();
        return true;

    case map::ItemType::Invul:
        player.give_invul();
        return true;

    case map::ItemType::Invis:
        player.give_invis();
        return true;

    case map::ItemType::Bottle:
        if (player.health() >= kHealthLimit) {
            return false;
        }
        player.heal_bottle();
        return true;

    case map::ItemType::Helmet:
        return player.add_armor_small();

    case map::ItemType::Jetpack:
        if (player.jet_fuel() >= PlayerState::kJetFuelMax) {
            return false;
        }
        player.refill_jetpack();
        return true;

    case map::ItemType::KeyRed:
        return player.give_key_red();

    case map::ItemType::KeyGreen:
        return player.give_key_green();

    case map::ItemType::KeyBlue:
        return player.give_key_blue();

    case map::ItemType::WeaponKnuckles:
        return apply_weapon_pickup(combat, WeaponId::Knuckles, AmmoType::Bullets, 0, true);

    case map::ItemType::WeaponPistol:
        return apply_weapon_pickup(combat, WeaponId::Pistol, AmmoType::Bullets, 0, true);

    case map::ItemType::WeaponSaw:
        return apply_weapon_pickup(combat, WeaponId::Saw, AmmoType::Bullets, 0, true);

    case map::ItemType::WeaponShotgun1:
        return apply_weapon_pickup(combat, WeaponId::Shotgun1, AmmoType::Shells, 4, true);

    case map::ItemType::WeaponShotgun2:
        return apply_weapon_pickup(combat, WeaponId::Shotgun2, AmmoType::Shells, 4, true);

    case map::ItemType::WeaponChaingun:
        return apply_weapon_pickup(combat, WeaponId::Chaingun, AmmoType::Bullets, 50, true);

    case map::ItemType::WeaponRocketLauncher:
        return apply_weapon_pickup(combat, WeaponId::RocketLauncher, AmmoType::Rockets, 2, true);

    case map::ItemType::WeaponPlasma:
        return apply_weapon_pickup(combat, WeaponId::Plasma, AmmoType::Cells, 40, true);

    case map::ItemType::WeaponBfg:
        return apply_weapon_pickup(combat, WeaponId::Bfg, AmmoType::Cells, 40, true);

    case map::ItemType::WeaponSuperChaingun:
        return apply_weapon_pickup(combat, WeaponId::SuperChaingun, AmmoType::Shells, 4, true);

    case map::ItemType::WeaponFlamethrower:
        return apply_weapon_pickup(combat, WeaponId::Flamethrower, AmmoType::Fuel, 100, true);

    case map::ItemType::AmmoBullets:
        if (combat.ammo[static_cast<std::size_t>(AmmoType::Bullets)] >=
            combat.max_ammo[static_cast<std::size_t>(AmmoType::Bullets)]) {
            return false;
        }
        inc_max(combat.ammo[static_cast<std::size_t>(AmmoType::Bullets)], 10,
                combat.max_ammo[static_cast<std::size_t>(AmmoType::Bullets)]);
        return true;

    case map::ItemType::AmmoBulletsBox:
        if (combat.ammo[static_cast<std::size_t>(AmmoType::Bullets)] >=
            combat.max_ammo[static_cast<std::size_t>(AmmoType::Bullets)]) {
            return false;
        }
        inc_max(combat.ammo[static_cast<std::size_t>(AmmoType::Bullets)], 50,
                combat.max_ammo[static_cast<std::size_t>(AmmoType::Bullets)]);
        return true;

    case map::ItemType::AmmoShells:
        if (combat.ammo[static_cast<std::size_t>(AmmoType::Shells)] >=
            combat.max_ammo[static_cast<std::size_t>(AmmoType::Shells)]) {
            return false;
        }
        inc_max(combat.ammo[static_cast<std::size_t>(AmmoType::Shells)], 4,
                combat.max_ammo[static_cast<std::size_t>(AmmoType::Shells)]);
        return true;

    case map::ItemType::AmmoShellsBox:
        if (combat.ammo[static_cast<std::size_t>(AmmoType::Shells)] >=
            combat.max_ammo[static_cast<std::size_t>(AmmoType::Shells)]) {
            return false;
        }
        inc_max(combat.ammo[static_cast<std::size_t>(AmmoType::Shells)], 25,
                combat.max_ammo[static_cast<std::size_t>(AmmoType::Shells)]);
        return true;

    case map::ItemType::AmmoRocket:
        if (combat.ammo[static_cast<std::size_t>(AmmoType::Rockets)] >=
            combat.max_ammo[static_cast<std::size_t>(AmmoType::Rockets)]) {
            return false;
        }
        inc_max(combat.ammo[static_cast<std::size_t>(AmmoType::Rockets)], 1,
                combat.max_ammo[static_cast<std::size_t>(AmmoType::Rockets)]);
        return true;

    case map::ItemType::AmmoRocketBox:
        if (combat.ammo[static_cast<std::size_t>(AmmoType::Rockets)] >=
            combat.max_ammo[static_cast<std::size_t>(AmmoType::Rockets)]) {
            return false;
        }
        inc_max(combat.ammo[static_cast<std::size_t>(AmmoType::Rockets)], 5,
                combat.max_ammo[static_cast<std::size_t>(AmmoType::Rockets)]);
        return true;

    case map::ItemType::AmmoCell:
        if (combat.ammo[static_cast<std::size_t>(AmmoType::Cells)] >=
            combat.max_ammo[static_cast<std::size_t>(AmmoType::Cells)]) {
            return false;
        }
        inc_max(combat.ammo[static_cast<std::size_t>(AmmoType::Cells)], 40,
                combat.max_ammo[static_cast<std::size_t>(AmmoType::Cells)]);
        return true;

    case map::ItemType::AmmoCellBig:
        if (combat.ammo[static_cast<std::size_t>(AmmoType::Cells)] >=
            combat.max_ammo[static_cast<std::size_t>(AmmoType::Cells)]) {
            return false;
        }
        inc_max(combat.ammo[static_cast<std::size_t>(AmmoType::Cells)], 100,
                combat.max_ammo[static_cast<std::size_t>(AmmoType::Cells)]);
        return true;

    case map::ItemType::AmmoFuelcan:
        if (combat.ammo[static_cast<std::size_t>(AmmoType::Fuel)] >=
            combat.max_ammo[static_cast<std::size_t>(AmmoType::Fuel)]) {
            return false;
        }
        inc_max(combat.ammo[static_cast<std::size_t>(AmmoType::Fuel)], 100,
                combat.max_ammo[static_cast<std::size_t>(AmmoType::Fuel)]);
        return true;

    case map::ItemType::AmmoBackpack:
        combat.max_ammo[static_cast<std::size_t>(AmmoType::Bullets)] = 200;
        combat.max_ammo[static_cast<std::size_t>(AmmoType::Shells)] = 50;
        combat.max_ammo[static_cast<std::size_t>(AmmoType::Rockets)] = 50;
        combat.max_ammo[static_cast<std::size_t>(AmmoType::Cells)] = 300;
        combat.max_ammo[static_cast<std::size_t>(AmmoType::Fuel)] = 100;
        inc_max(combat.ammo[static_cast<std::size_t>(AmmoType::Bullets)], 10,
                combat.max_ammo[static_cast<std::size_t>(AmmoType::Bullets)]);
        inc_max(combat.ammo[static_cast<std::size_t>(AmmoType::Shells)], 4,
                combat.max_ammo[static_cast<std::size_t>(AmmoType::Shells)]);
        inc_max(combat.ammo[static_cast<std::size_t>(AmmoType::Rockets)], 1,
                combat.max_ammo[static_cast<std::size_t>(AmmoType::Rockets)]);
        inc_max(combat.ammo[static_cast<std::size_t>(AmmoType::Cells)], 40,
                combat.max_ammo[static_cast<std::size_t>(AmmoType::Cells)]);
        inc_max(combat.ammo[static_cast<std::size_t>(AmmoType::Fuel)], 100,
                combat.max_ammo[static_cast<std::size_t>(AmmoType::Fuel)]);
        return true;

    default:
        return false;
    }
}

bool should_remove_item(const WorldItem& item, map::ItemType type, const PlayerState& player,
                        const GameRules& rules) {
    switch (type) {
    case map::ItemType::KeyRed:
    case map::ItemType::KeyGreen:
    case map::ItemType::KeyBlue:
        return !rules.shared_keys();

    default:
        break;
    }

    if (rules.weapons_stay && item.respawnable && map::item_is_weapon(type)) {
        const auto weapon_index = static_cast<std::size_t>(
            [&]() -> WeaponId {
                switch (type) {
                case map::ItemType::WeaponKnuckles:
                    return WeaponId::Knuckles;
                case map::ItemType::WeaponPistol:
                    return WeaponId::Pistol;
                case map::ItemType::WeaponSaw:
                    return WeaponId::Saw;
                case map::ItemType::WeaponShotgun1:
                    return WeaponId::Shotgun1;
                case map::ItemType::WeaponShotgun2:
                    return WeaponId::Shotgun2;
                case map::ItemType::WeaponChaingun:
                    return WeaponId::Chaingun;
                case map::ItemType::WeaponRocketLauncher:
                    return WeaponId::RocketLauncher;
                case map::ItemType::WeaponPlasma:
                    return WeaponId::Plasma;
                case map::ItemType::WeaponBfg:
                    return WeaponId::Bfg;
                case map::ItemType::WeaponSuperChaingun:
                    return WeaponId::SuperChaingun;
                case map::ItemType::WeaponFlamethrower:
                    return WeaponId::Flamethrower;
                default:
                    return WeaponId::Knuckles;
                }
            }());
        if (player.combat().owned[weapon_index]) {
            return false;
        }
    }

    return true;
}

void tick_falling_item(WorldItem& item, const MapCollision& collision) {
    switch (collision.vertical_lift_at(item.x, item.y, item.width, item.height)) {
    case -1:
        item.vel_y -= 1;
        if (item.vel_y < -5) {
            item.vel_y += 1;
        }
        break;
    case 1:
        if (item.vel_y > 5) {
            item.vel_y -= 1;
        }
        item.vel_y += 1;
        break;
    default:
        item.vel_y += 1;
        if (item.vel_y > PlayerState::kMaxYVel) {
            item.vel_y = PlayerState::kMaxYVel;
        }
        break;
    }

    switch (collision.horizontal_lift_at(item.x, item.y, item.width, item.height)) {
    case -1:
        item.vel_x -= 3;
        if (item.vel_x < -9) {
            item.vel_x += 3;
        }
        break;
    case 1:
        item.vel_x += 3;
        if (item.vel_x > 9) {
            item.vel_x -= 3;
        }
        break;
    default:
        break;
    }

    float x = item.x;
    float y = item.y;
    const auto state = collision.move_object(x, y, item.width, item.height, item.vel_x, item.vel_y,
                                             true);
    item.x = x;
    item.y = y;

    if ((state & MOVE_HITLAND) != 0 || (state & MOVE_HITCEIL) != 0) {
        item.vel_y = 0;
    }
    if ((state & MOVE_HITWALL) != 0) {
        item.vel_x = 0;
    }

    // Legacy g_items.pas: air resistance on horizontal drift every other tick.
    if ((item.anim_tick % 2) == 0) {
        item.vel_x = z_dec(item.vel_x, 1);
    }
}

} // namespace

void ItemSystem::spawn_monster_drop(map::ItemType type, float center_x, float center_y, int vel_x,
                                    int vel_y) {
    if (type == map::ItemType::None) {
        return;
    }

    WorldItem item;
    item.type = type;
    const auto dims = map::item_dimensions(type);
    item.x = center_x - dims.width * 0.5f;
    item.y = center_y - dims.height * 0.5f;
    item.spawn_x = item.x;
    item.spawn_y = item.y;
    item.width = dims.width;
    item.height = dims.height;
    item.fall = true;
    item.respawnable = false;
    item.dropped = true;
    item.vel_x = vel_x;
    item.vel_y = vel_y;
    item.active = true;
    items_.push_back(item);
}

void ItemSystem::reset(const map::MapDocument& map, const GameRules& rules) {
    rules_ = rules;
    items_.clear();
    for (const auto& map_item : map.items) {
        if (map_item.type == map::ItemType::None) {
            continue;
        }
        if (!rules.is_multiplayer() && map_item.only_dm) {
            continue;
        }

        WorldItem item;
        item.type = map_item.type;
        item.x = static_cast<float>(map_item.position.x);
        item.y = static_cast<float>(map_item.position.y);
        item.spawn_x = item.x;
        item.spawn_y = item.y;
        item.fall = map_item.fall;
        const auto dims = map::item_dimensions(map_item.type);
        item.width = dims.width;
        item.height = dims.height;
        if (rules.items_respawn()) {
            item.respawnable = map::item_respawns_in_multiplayer(map_item.type);
        } else {
            item.respawnable = map::item_respawns_in_single_player(map_item.type);
        }
        items_.push_back(item);
    }
}

void ItemSystem::tick(PlayerState& player, const MapCollision* collision, EventBus* events) {
    for (auto& item : items_) {
        if (!item.active && item.respawnable && item.respawn_countdown > 0) {
            --item.respawn_countdown;
            if (item.respawn_countdown == 0) {
                item.active = true;
                item.x = item.spawn_x;
                item.y = item.spawn_y;
                item.vel_x = 0;
                item.vel_y = 0;
                item.respawn_anim_tick = kItemRespawnAnimTotalTicks;
                if (events != nullptr) {
                    events->publish(events::ItemRespawned{item.x, item.y});
                }
            }
        }

        if (item.respawn_anim_tick > 0) {
            --item.respawn_anim_tick;
        }

        if (item.active) {
            ++item.anim_tick;
            if (item.fall && collision != nullptr) {
                tick_falling_item(item, *collision);
            }
        }
    }

    if (!player.alive()) {
        return;
    }

    for (auto& item : items_) {
        if (!item.active) {
            continue;
        }

        if (!rects_overlap(player.x, player.y, player.width, player.height, item.x, item.y,
                           item.width, item.height)) {
            continue;
        }

        if (!apply_item_effect(item.type, player)) {
            continue;
        }

        const bool remove = should_remove_item(item, item.type, player, rules_);
        if (remove) {
            if (item.respawnable) {
                item.active = false;
                item.respawn_countdown = kDefaultItemRespawnTicks;
            } else {
                item.active = false;
            }
        }

        if (events != nullptr) {
            events->publish(events::ItemPickedUp{static_cast<std::uint8_t>(item.type)});
        }
    }
}

} // namespace d2df::sim
