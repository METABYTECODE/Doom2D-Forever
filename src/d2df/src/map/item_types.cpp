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
    if (name == "ITEM_KEY_RED") {
        return ItemType::KeyRed;
    }
    if (name == "ITEM_KEY_GREEN") {
        return ItemType::KeyGreen;
    }
    if (name == "ITEM_KEY_BLUE") {
        return ItemType::KeyBlue;
    }
    if (name == "ITEM_SUIT") {
        return ItemType::Suit;
    }
    if (name == "ITEM_OXYGEN") {
        return ItemType::Oxygen;
    }
    if (name == "ITEM_INVUL") {
        return ItemType::Invul;
    }
    if (name == "ITEM_INVIS") {
        return ItemType::Invis;
    }
    if (name == "ITEM_BOTTLE") {
        return ItemType::Bottle;
    }
    if (name == "ITEM_HELMET") {
        return ItemType::Helmet;
    }
    if (name == "ITEM_JETPACK") {
        return ItemType::Jetpack;
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

bool item_is_weapon(ItemType type) {
    switch (type) {
    case ItemType::WeaponKnuckles:
    case ItemType::WeaponPistol:
    case ItemType::WeaponSaw:
    case ItemType::WeaponShotgun1:
    case ItemType::WeaponShotgun2:
    case ItemType::WeaponChaingun:
    case ItemType::WeaponRocketLauncher:
    case ItemType::WeaponPlasma:
    case ItemType::WeaponBfg:
    case ItemType::WeaponSuperChaingun:
    case ItemType::WeaponFlamethrower:
        return true;
    default:
        return false;
    }
}

bool item_respawns_in_multiplayer(ItemType type) {
    if (type == ItemType::None) {
        return false;
    }
    switch (type) {
    case ItemType::KeyRed:
    case ItemType::KeyGreen:
    case ItemType::KeyBlue:
        return false;
    default:
        return true;
    }
}

bool item_respawns_in_single_player(ItemType type) {
    switch (type) {
    case ItemType::MedkitSmall:
    case ItemType::MedkitLarge:
    case ItemType::MedkitBlack:
    case ItemType::AmmoBullets:
    case ItemType::AmmoBulletsBox:
    case ItemType::AmmoShells:
    case ItemType::AmmoShellsBox:
    case ItemType::AmmoRocket:
    case ItemType::AmmoRocketBox:
    case ItemType::AmmoCell:
    case ItemType::AmmoCellBig:
    case ItemType::AmmoFuelcan:
        return true;
    default:
        return false;
    }
}

const char* item_texture_asset_id(ItemType type) {
    return item_sprite(type).texture_id;
}

ItemSprite item_sprite(ItemType type) {
    switch (type) {
    case ItemType::MedkitSmall:
        return {"tex.ui.med1", 0, 0};
    case ItemType::MedkitLarge:
        return {"tex.ui.med2", 0, 0};
    case ItemType::MedkitBlack:
        return {"tex.ui.bmed", 0, 0};
    case ItemType::ArmorGreen:
    case ItemType::ArmorBlue:
        return {type == ItemType::ArmorGreen ? "tex.ui.armorgreen" : "tex.ui.armorblue", 32, 16, 3,
                20};
    case ItemType::SphereBlue:
    case ItemType::SphereWhite:
        return {type == ItemType::SphereBlue ? "tex.ui.sblue" : "tex.ui.swhite", 32, 32, 4,
                type == ItemType::SphereBlue ? 15 : 20};
    case ItemType::Invul:
    case ItemType::Invis:
        return {type == ItemType::Invul ? "tex.ui.invul" : "tex.ui.invis", 32, 32, 4, 20};
    case ItemType::WeaponKnuckles:
        return {"tex.ui.knuckles", 0, 0};
    case ItemType::WeaponSaw:
        return {"tex.ui.saw", 0, 0};
    case ItemType::WeaponShotgun1:
        return {"tex.ui.shotgun1", 0, 0};
    case ItemType::WeaponShotgun2:
        return {"tex.ui.shotgun2", 0, 0};
    case ItemType::WeaponChaingun:
        return {"tex.ui.mgun_2", 0, 0};
    case ItemType::WeaponRocketLauncher:
        return {"tex.ui.rlauncher", 0, 0};
    case ItemType::WeaponPlasma:
        return {"tex.ui.pgun", 0, 0};
    case ItemType::WeaponBfg:
        return {"tex.ui.bfg_2", 0, 0};
    case ItemType::WeaponSuperChaingun:
        return {"tex.ui.schaingun", 0, 0};
    case ItemType::WeaponFlamethrower:
        return {"tex.ui.flamethrower", 0, 0};
    case ItemType::WeaponPistol:
        return {"tex.ui.pistol", 0, 0};
    case ItemType::AmmoBullets:
        return {"tex.ui.clip", 0, 0};
    case ItemType::AmmoBulletsBox:
        return {"tex.ui.ammo", 0, 0};
    case ItemType::AmmoShells:
        return {"tex.ui.shell1_2", 0, 0};
    case ItemType::AmmoShellsBox:
        return {"tex.ui.shell2_2", 0, 0};
    case ItemType::AmmoRocket:
        return {"tex.ui.rocket", 0, 0};
    case ItemType::AmmoRocketBox:
        return {"tex.ui.rockets", 0, 0};
    case ItemType::AmmoCell:
        return {"tex.ui.cell", 0, 0};
    case ItemType::AmmoCellBig:
        return {"tex.ui.cell2", 0, 0};
    case ItemType::AmmoFuelcan:
        return {"tex.ui.fuelcan", 0, 0};
    case ItemType::AmmoBackpack:
        return {"tex.ui.bpack", 0, 0};
    case ItemType::KeyRed:
        return {"tex.ui.keyr", 0, 0};
    case ItemType::KeyGreen:
        return {"tex.ui.keyg", 0, 0};
    case ItemType::KeyBlue:
        return {"tex.ui.keyb", 0, 0};
    case ItemType::Bottle:
        return {"tex.ui.bottle", 16, 32, 4, 20};
    case ItemType::Helmet:
        return {"tex.ui.helmet", 16, 16, 4, 20};
    case ItemType::Jetpack:
        return {"tex.ui.jetpack", 32, 32, 3, 15};
    case ItemType::Suit:
        return {"tex.ui.suit", 0, 0};
    case ItemType::Oxygen:
        return {"tex.ui.oxygen", 0, 0};
    default:
        return {nullptr, 0, 0};
    }
}

const char* item_pickup_sfx(ItemType type) {
    if (item_is_weapon(type)) {
        return "sfx.world.getweapon";
    }
    switch (type) {
    case ItemType::Invul:
    case ItemType::Invis:
    case ItemType::Suit:
    case ItemType::SphereBlue:
    case ItemType::SphereWhite:
        return "sfx.world.getpowerup";
    default:
        return "sfx.world.getitem";
    }
}

} // namespace d2df::map
