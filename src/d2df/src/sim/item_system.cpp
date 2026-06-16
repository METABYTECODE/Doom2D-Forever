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

    case map::ItemType::SphereBlue:
        return player.add_health(100, kHealthLimit);

    case map::ItemType::SphereWhite:
        if (player.health() >= kHealthLimit) {
            return false;
        }
        player.set_health(kHealthLimit);
        return true;

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

} // namespace

void ItemSystem::reset(const map::MapDocument& map, bool single_player) {
    items_.clear();
    for (const auto& map_item : map.items) {
        if (map_item.type == map::ItemType::None) {
            continue;
        }
        if (single_player && map_item.only_dm) {
            continue;
        }

        WorldItem item;
        item.type = map_item.type;
        item.x = static_cast<float>(map_item.position.x);
        item.y = static_cast<float>(map_item.position.y);
        const auto dims = map::item_dimensions(map_item.type);
        item.width = dims.width;
        item.height = dims.height;
        items_.push_back(item);
    }
}

void ItemSystem::tick(PlayerState& player, EventBus* events) {
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

        item.active = false;
        if (events != nullptr) {
            events->publish(events::ItemPickedUp{static_cast<std::uint8_t>(item.type)});
        }
    }
}

} // namespace d2df::sim
