#pragma once

#include <cstddef>
#include <cstdint>

namespace d2df::sim {

constexpr std::size_t kWeaponSlotCount = 11;
constexpr std::size_t kAmmoTypeCount = 5;

enum class WeaponId : std::uint8_t {
    Knuckles = 0,
    Saw = 1,
    Pistol = 2,
    Shotgun1 = 3,
    Shotgun2 = 4,
    Chaingun = 5,
    RocketLauncher = 6,
    Plasma = 7,
    Bfg = 8,
    SuperChaingun = 9,
    Flamethrower = 10,
    First = Knuckles,
    Last = Flamethrower,
};

enum class AmmoType : std::uint8_t {
    Bullets = 0,
    Shells = 1,
    Rockets = 2,
    Cells = 3,
    Fuel = 4,
};

struct PlayerCombat {
    WeaponId current_weapon = WeaponId::Pistol;
    bool owned[kWeaponSlotCount] = {};
    int ammo[kAmmoTypeCount] = {};
    int max_ammo[kAmmoTypeCount] = {};
    int reloading[kWeaponSlotCount] = {};
    bool facing_left = false;

    /// -1 = idle; counts down each tick until BFG shot is released (legacy: starts at 17).
    int bfg_charge_ticks = -1;
    float bfg_muzzle_x = 0.0f;
    float bfg_muzzle_y = 0.0f;
    float bfg_aim_x = 0.0f;
    float bfg_aim_y = 0.0f;

    void reset_single_player_loadout();
    void tick_reload();

    [[nodiscard]] int ammo_for_current_weapon() const;
    [[nodiscard]] bool can_select_weapon(WeaponId weapon) const;
    [[nodiscard]] bool is_reloading(WeaponId weapon) const;
    [[nodiscard]] bool has_ammo_for_weapon(WeaponId weapon) const;

    void select_weapon(WeaponId weapon);
    void cycle_weapon(int direction);
};

[[nodiscard]] int weapon_reload_ticks(WeaponId weapon);
[[nodiscard]] AmmoType weapon_ammo_type(WeaponId weapon);
[[nodiscard]] const char* weapon_display_name(WeaponId weapon);
[[nodiscard]] const char* weapon_fire_sfx(WeaponId weapon);

} // namespace d2df::sim
