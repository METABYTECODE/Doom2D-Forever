#include <d2df/map/item_types.hpp>

namespace d2df::map {

ItemType item_type_from_name(std::string_view name) {
    if (name == "ITEM_MEDKIT_SMALL") {
        return ItemType::MedkitSmall;
    }
    if (name == "ITEM_MEDKIT_LARGE") {
        return ItemType::MedkitLarge;
    }
    if (name == "ITEM_MEDKIT_BLACK") {
        return ItemType::MedkitBlack;
    }
    if (name == "ITEM_ARMOR_GREEN") {
        return ItemType::ArmorGreen;
    }
    if (name == "ITEM_ARMOR_BLUE") {
        return ItemType::ArmorBlue;
    }
    if (name == "ITEM_SPHERE_BLUE") {
        return ItemType::SphereBlue;
    }
    if (name == "ITEM_SPHERE_WHITE") {
        return ItemType::SphereWhite;
    }
    if (name == "ITEM_WEAPON_SAW") {
        return ItemType::WeaponSaw;
    }
    if (name == "ITEM_WEAPON_SHOTGUN1") {
        return ItemType::WeaponShotgun1;
    }
    if (name == "ITEM_WEAPON_SHOTGUN2") {
        return ItemType::WeaponShotgun2;
    }
    if (name == "ITEM_WEAPON_CHAINGUN") {
        return ItemType::WeaponChaingun;
    }
    if (name == "ITEM_WEAPON_ROCKETLAUNCHER") {
        return ItemType::WeaponRocketLauncher;
    }
    if (name == "ITEM_WEAPON_PLASMA") {
        return ItemType::WeaponPlasma;
    }
    if (name == "ITEM_WEAPON_BFG") {
        return ItemType::WeaponBfg;
    }
    if (name == "ITEM_WEAPON_SUPERCHAINGUN" || name == "ITEM_WEAPON_SUPERPULEMET") {
        return ItemType::WeaponSuperChaingun;
    }
    if (name == "ITEM_WEAPON_FLAMETHROWER") {
        return ItemType::WeaponFlamethrower;
    }
    if (name == "ITEM_WEAPON_KNUCKLES") {
        return ItemType::WeaponKnuckles;
    }
    if (name == "ITEM_WEAPON_PISTOL") {
        return ItemType::WeaponPistol;
    }
    if (name == "ITEM_AMMO_BULLETS") {
        return ItemType::AmmoBullets;
    }
    if (name == "ITEM_AMMO_BULLETS_BOX") {
        return ItemType::AmmoBulletsBox;
    }
    if (name == "ITEM_AMMO_SHELLS") {
        return ItemType::AmmoShells;
    }
    if (name == "ITEM_AMMO_SHELLS_BOX") {
        return ItemType::AmmoShellsBox;
    }
    if (name == "ITEM_AMMO_ROCKET") {
        return ItemType::AmmoRocket;
    }
    if (name == "ITEM_AMMO_ROCKET_BOX") {
        return ItemType::AmmoRocketBox;
    }
    if (name == "ITEM_AMMO_CELL") {
        return ItemType::AmmoCell;
    }
    if (name == "ITEM_AMMO_CELL_BIG") {
        return ItemType::AmmoCellBig;
    }
    if (name == "ITEM_AMMO_FUELCAN") {
        return ItemType::AmmoFuelcan;
    }
    if (name == "ITEM_AMMO_BACKPACK") {
        return ItemType::AmmoBackpack;
    }
    return ItemType::None;
}

ItemDimensions item_dimensions(ItemType type) {
    switch (type) {
    case ItemType::MedkitSmall:
        return {14.0f, 15.0f};
    case ItemType::MedkitLarge:
    case ItemType::MedkitBlack:
        return {28.0f, 19.0f};
    case ItemType::ArmorGreen:
    case ItemType::ArmorBlue:
        return {31.0f, 16.0f};
    case ItemType::SphereBlue:
    case ItemType::SphereWhite:
    case ItemType::Invul:
    case ItemType::Invis:
        return {25.0f, 25.0f};
    case ItemType::WeaponSaw:
        return {62.0f, 24.0f};
    case ItemType::WeaponShotgun1:
        return {63.0f, 12.0f};
    case ItemType::WeaponShotgun2:
        return {54.0f, 13.0f};
    case ItemType::WeaponChaingun:
    case ItemType::WeaponPlasma:
    case ItemType::WeaponSuperChaingun:
        return {54.0f, 16.0f};
    case ItemType::WeaponRocketLauncher:
        return {62.0f, 16.0f};
    case ItemType::WeaponBfg:
        return {61.0f, 36.0f};
    case ItemType::WeaponFlamethrower:
        return {53.0f, 20.0f};
    case ItemType::WeaponPistol:
        return {43.0f, 16.0f};
    case ItemType::AmmoBullets:
        return {9.0f, 11.0f};
    case ItemType::AmmoBulletsBox:
        return {28.0f, 16.0f};
    case ItemType::AmmoShells:
        return {15.0f, 7.0f};
    case ItemType::AmmoShellsBox:
        return {32.0f, 12.0f};
    case ItemType::AmmoRocket:
        return {12.0f, 27.0f};
    case ItemType::AmmoRocketBox:
        return {54.0f, 21.0f};
    case ItemType::AmmoCell:
        return {15.0f, 12.0f};
    case ItemType::AmmoCellBig:
        return {32.0f, 21.0f};
    case ItemType::AmmoFuelcan:
        return {13.0f, 20.0f};
    case ItemType::AmmoBackpack:
        return {22.0f, 29.0f};
    case ItemType::KeyRed:
    case ItemType::KeyGreen:
    case ItemType::KeyBlue:
    case ItemType::Helmet:
        return {16.0f, 16.0f};
    case ItemType::Bottle:
        return {14.0f, 18.0f};
    case ItemType::Jetpack:
        return {32.0f, 24.0f};
    default:
        return {16.0f, 16.0f};
    }
}

} // namespace d2df::map
