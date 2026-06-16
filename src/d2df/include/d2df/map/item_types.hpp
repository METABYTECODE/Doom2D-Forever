#pragma once

#include <cstdint>
#include <string_view>

namespace d2df::map {

enum class ItemType : std::uint8_t {
    None = 0,
    MedkitSmall = 1,
    MedkitLarge = 2,
    MedkitBlack = 3,
    ArmorGreen = 4,
    ArmorBlue = 5,
    SphereBlue = 6,
    SphereWhite = 7,
    Suit = 8,
    Oxygen = 9,
    Invul = 10,
    WeaponSaw = 11,
    WeaponShotgun1 = 12,
    WeaponShotgun2 = 13,
    WeaponChaingun = 14,
    WeaponRocketLauncher = 15,
    WeaponPlasma = 16,
    WeaponBfg = 17,
    WeaponSuperChaingun = 18,
    AmmoBullets = 19,
    AmmoBulletsBox = 20,
    AmmoShells = 21,
    AmmoShellsBox = 22,
    AmmoRocket = 23,
    AmmoRocketBox = 24,
    AmmoCell = 25,
    AmmoCellBig = 26,
    AmmoBackpack = 27,
    KeyRed = 28,
    KeyGreen = 29,
    KeyBlue = 30,
    WeaponKnuckles = 31,
    WeaponPistol = 32,
    Bottle = 33,
    Helmet = 34,
    Jetpack = 35,
    Invis = 36,
    WeaponFlamethrower = 37,
    AmmoFuelcan = 38,
};

enum class ItemOption : std::uint8_t {
    None = 0,
    OnlyDm = 1,
    Fall = 2,
};

struct ItemDimensions {
    float width = 16.0f;
    float height = 16.0f;
};

[[nodiscard]] ItemType item_type_from_name(std::string_view name);
[[nodiscard]] ItemDimensions item_dimensions(ItemType type);

} // namespace d2df::map
