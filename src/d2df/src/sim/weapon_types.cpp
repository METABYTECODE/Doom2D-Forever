#include <d2df/sim/weapon_types.hpp>

#include <algorithm>

namespace d2df::sim {
namespace {

constexpr int kDefaultMaxAmmo[] = {200, 50, 50, 300, 100};
constexpr int kWeaponReloadTicks[] = {5, 2, 6, 18, 36, 2, 12, 2, 14, 2, 2};

AmmoType ammo_for_weapon(WeaponId weapon) {
    switch (weapon) {
    case WeaponId::Pistol:
    case WeaponId::Chaingun:
        return AmmoType::Bullets;
    case WeaponId::Shotgun1:
    case WeaponId::Shotgun2:
    case WeaponId::SuperChaingun:
        return AmmoType::Shells;
    case WeaponId::RocketLauncher:
        return AmmoType::Rockets;
    case WeaponId::Plasma:
    case WeaponId::Bfg:
        return AmmoType::Cells;
    case WeaponId::Flamethrower:
        return AmmoType::Fuel;
    default:
        return AmmoType::Bullets;
    }
}

bool weapon_needs_ammo(WeaponId weapon) {
    switch (weapon) {
    case WeaponId::Knuckles:
    case WeaponId::Saw:
        return false;
    default:
        return true;
    }
}

} // namespace

int weapon_reload_ticks(WeaponId weapon) {
    const auto index = static_cast<std::size_t>(weapon);
    if (index >= kWeaponSlotCount) {
        return 0;
    }
    return kWeaponReloadTicks[index];
}

AmmoType weapon_ammo_type(WeaponId weapon) {
    return ammo_for_weapon(weapon);
}

const char* weapon_display_name(WeaponId weapon) {
    switch (weapon) {
    case WeaponId::Knuckles:
        return "Knuckles";
    case WeaponId::Saw:
        return "Chainsaw";
    case WeaponId::Pistol:
        return "Pistol";
    case WeaponId::Shotgun1:
        return "Shotgun";
    case WeaponId::Shotgun2:
        return "Super Shotgun";
    case WeaponId::Chaingun:
        return "Chaingun";
    case WeaponId::RocketLauncher:
        return "Rocket";
    case WeaponId::Plasma:
        return "Plasma";
    case WeaponId::Bfg:
        return "BFG";
    case WeaponId::SuperChaingun:
        return "Super Chaingun";
    case WeaponId::Flamethrower:
        return "Flamethrower";
    default:
        return "?";
    }
}

void PlayerCombat::reset_single_player_loadout() {
    current_weapon = WeaponId::Pistol;
    for (auto& owned_weapon : owned) {
        owned_weapon = false;
    }
    owned[static_cast<std::size_t>(WeaponId::Knuckles)] = true;
    owned[static_cast<std::size_t>(WeaponId::Pistol)] = true;

    for (std::size_t i = 0; i < kAmmoTypeCount; ++i) {
        ammo[i] = 0;
        max_ammo[i] = kDefaultMaxAmmo[i];
    }
    ammo[static_cast<std::size_t>(AmmoType::Bullets)] = 50;

    for (auto& reload : reloading) {
        reload = 0;
    }
    bfg_charge_ticks = -1;
    facing_left = false;
}

void PlayerCombat::tick_reload() {
    for (auto& reload : reloading) {
        if (reload > 0) {
            --reload;
        }
    }
}

int PlayerCombat::ammo_for_current_weapon() const {
    if (!weapon_needs_ammo(current_weapon)) {
        return 0;
    }
    const auto ammo_type = ammo_for_weapon(current_weapon);
    return ammo[static_cast<std::size_t>(ammo_type)];
}

bool PlayerCombat::can_select_weapon(WeaponId weapon) const {
    const auto index = static_cast<std::size_t>(weapon);
    if (index >= kWeaponSlotCount) {
        return false;
    }
    return owned[index];
}

bool PlayerCombat::is_reloading(WeaponId weapon) const {
    const auto index = static_cast<std::size_t>(weapon);
    if (index >= kWeaponSlotCount) {
        return false;
    }
    return reloading[index] > 0;
}

bool PlayerCombat::has_ammo_for_weapon(WeaponId weapon) const {
    if (!weapon_needs_ammo(weapon)) {
        return true;
    }
    const auto ammo_type = ammo_for_weapon(weapon);
    if (weapon == WeaponId::Shotgun2) {
        return ammo[static_cast<std::size_t>(ammo_type)] >= 2;
    }
    if (weapon == WeaponId::Bfg) {
        return ammo[static_cast<std::size_t>(ammo_type)] >= 40;
    }
    return ammo[static_cast<std::size_t>(ammo_type)] > 0;
}

void PlayerCombat::select_weapon(WeaponId weapon) {
    if (!can_select_weapon(weapon)) {
        return;
    }
    current_weapon = weapon;
}

void PlayerCombat::cycle_weapon(int direction) {
    if (direction == 0) {
        return;
    }

    int index = static_cast<int>(current_weapon);
    for (int step = 0; step < static_cast<int>(kWeaponSlotCount); ++step) {
        index += direction;
        if (index < 0) {
            index = static_cast<int>(WeaponId::Last);
        } else if (index > static_cast<int>(WeaponId::Last)) {
            index = static_cast<int>(WeaponId::First);
        }
        const auto weapon = static_cast<WeaponId>(index);
        if (can_select_weapon(weapon)) {
            current_weapon = weapon;
            return;
        }
    }
}

} // namespace d2df::sim
