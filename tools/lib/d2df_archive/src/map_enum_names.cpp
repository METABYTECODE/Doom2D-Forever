#include <d2df/tools/map_enum_names.hpp>

namespace d2df::tools {

namespace {

std::string enum_name(const char* table[], std::size_t count, int value) {
    if (value < 0 || static_cast<std::size_t>(value) >= count) {
        return "UNKNOWN_" + std::to_string(value);
    }
    return table[value];
}

} // namespace

std::vector<std::string> bitset_names(const std::pair<std::uint16_t, const char*>* flags,
                                      std::size_t count, std::uint16_t value) {
    std::vector<std::string> out;
    for (std::size_t i = 0; i < count; ++i) {
        if ((value & flags[i].first) != 0) {
            out.emplace_back(flags[i].second);
        }
    }
    if (out.empty()) {
        out.emplace_back("NONE");
    }
    return out;
}

namespace {

const char* kDirTypes[] = {"DIR_LEFT", "DIR_RIGHT", "DIR_PRESERVE", "DIR_REVERSE"};

const char* kItems[] = {
    "ITEM_NONE",           "ITEM_MEDKIT_SMALL",     "ITEM_MEDKIT_LARGE",    "ITEM_MEDKIT_BLACK",
    "ITEM_ARMOR_GREEN",    "ITEM_ARMOR_BLUE",       "ITEM_SPHERE_BLUE",     "ITEM_SPHERE_WHITE",
    "ITEM_SUIT",           "ITEM_OXYGEN",           "ITEM_INVUL",           "ITEM_WEAPON_SAW",
    "ITEM_WEAPON_SHOTGUN1","ITEM_WEAPON_SHOTGUN2",  "ITEM_WEAPON_CHAINGUN", "ITEM_WEAPON_ROCKETLAUNCHER",
    "ITEM_WEAPON_PLASMA",  "ITEM_WEAPON_BFG",       "ITEM_WEAPON_SUPERCHAINGUN", "ITEM_AMMO_BULLETS",
    "ITEM_AMMO_BULLETS_BOX","ITEM_AMMO_SHELLS",     "ITEM_AMMO_SHELLS_BOX", "ITEM_AMMO_ROCKET",
    "ITEM_AMMO_ROCKET_BOX","ITEM_AMMO_CELL",        "ITEM_AMMO_CELL_BIG",   "ITEM_AMMO_BACKPACK",
    "ITEM_KEY_RED",        "ITEM_KEY_GREEN",        "ITEM_KEY_BLUE",        "ITEM_WEAPON_KNUCKLES",
    "ITEM_WEAPON_PISTOL",  "ITEM_BOTTLE",           "ITEM_HELMET",          "ITEM_JETPACK",
    "ITEM_INVIS",          "ITEM_WEAPON_FLAMETHROWER","ITEM_AMMO_FUELCAN",
};

const char* kMonsters[] = {
    "MONSTER_NONE", "MONSTER_DEMON", "MONSTER_IMP", "MONSTER_ZOMBY", "MONSTER_SERG", "MONSTER_CYBER",
    "MONSTER_CGUN", "MONSTER_BARON", "MONSTER_KNIGHT", "MONSTER_CACO", "MONSTER_SOUL", "MONSTER_PAIN",
    "MONSTER_SPIDER", "MONSTER_BSP", "MONSTER_MANCUB", "MONSTER_SKEL", "MONSTER_VILE", "MONSTER_FISH",
    "MONSTER_BARREL", "MONSTER_ROBO", "MONSTER_MAN",
};

const char* kAreas[] = {
    "AREA_NONE", "AREA_PLAYERPOINT1", "AREA_PLAYERPOINT2", "AREA_DMPOINT", "AREA_REDFLAG",
    "AREA_BLUEFLAG", "AREA_DOMFLAG", "AREA_REDTEAMPOINT", "AREA_BLUETEAMPOINT",
};

const char* kTriggers[] = {
    "TRIGGER_NONE", "TRIGGER_EXIT", "TRIGGER_TELEPORT", "TRIGGER_OPENDOOR", "TRIGGER_CLOSEDOOR",
    "TRIGGER_DOOR", "TRIGGER_DOOR5", "TRIGGER_CLOSETRAP", "TRIGGER_TRAP", "TRIGGER_PRESS",
    "TRIGGER_SECRET", "TRIGGER_LIFTUP", "TRIGGER_LIFTDOWN", "TRIGGER_LIFT", "TRIGGER_TEXTURE",
    "TRIGGER_ON", "TRIGGER_OFF", "TRIGGER_ONOFF", "TRIGGER_SOUND", "TRIGGER_SPAWNMONSTER",
    "TRIGGER_SPAWNITEM", "TRIGGER_MUSIC", "TRIGGER_PUSH", "TRIGGER_SCORE", "TRIGGER_MESSAGE",
    "TRIGGER_DAMAGE", "TRIGGER_HEALTH", "TRIGGER_SHOT", "TRIGGER_EFFECT", "TRIGGER_SCRIPT",
};

const std::pair<std::uint16_t, const char*> kPanelTypes[] = {
    {1, "PANEL_WALL"},       {2, "PANEL_BACK"},      {4, "PANEL_FORE"},       {8, "PANEL_WATER"},
    {16, "PANEL_ACID1"},     {32, "PANEL_ACID2"},    {64, "PANEL_STEP"},      {128, "PANEL_LIFTUP"},
    {256, "PANEL_LIFTDOWN"}, {512, "PANEL_OPENDOOR"},{1024, "PANEL_CLOSEDOOR"},{2048, "PANEL_BLOCKMON"},
    {4096, "PANEL_LIFTLEFT"},{8192, "PANEL_LIFTRIGHT"},
};

const std::pair<std::uint16_t, const char*> kPanelFlags[] = {
    {1, "PANEL_FLAG_BLENDING"},
    {2, "PANEL_FLAG_HIDE"},
    {4, "PANEL_FLAG_WATERTEXTURES"},
};

const std::pair<std::uint16_t, const char*> kActivateTypes[] = {
    {1, "ACTIVATE_PLAYERCOLLIDE"},  {2, "ACTIVATE_MONSTERCOLLIDE"}, {4, "ACTIVATE_PLAYERPRESS"},
    {8, "ACTIVATE_MONSTERPRESS"},     {16, "ACTIVATE_SHOT"},          {32, "ACTIVATE_NOMONSTER"},
};

const std::pair<std::uint16_t, const char*> kKeys[] = {
    {1, "KEY_RED"}, {2, "KEY_GREEN"}, {4, "KEY_BLUE"}, {8, "KEY_REDTEAM"}, {16, "KEY_BLUETEAM"},
};

const std::pair<std::uint16_t, const char*> kItemOptions[] = {
    {1, "ITEM_OPTION_ONLYDM"},
    {2, "ITEM_OPTION_FALL"},
};

} // namespace

std::string dir_type_name(std::uint8_t value) {
    return enum_name(kDirTypes, sizeof(kDirTypes) / sizeof(kDirTypes[0]), value);
}

std::string item_type_name(std::uint8_t value) {
    return enum_name(kItems, sizeof(kItems) / sizeof(kItems[0]), value);
}

std::string monster_type_name(std::uint8_t value) {
    return enum_name(kMonsters, sizeof(kMonsters) / sizeof(kMonsters[0]), value);
}

std::string area_type_name(std::uint8_t value) {
    return enum_name(kAreas, sizeof(kAreas) / sizeof(kAreas[0]), value);
}

std::string trigger_type_name(std::uint8_t value) {
    return enum_name(kTriggers, sizeof(kTriggers) / sizeof(kTriggers[0]), value);
}

std::vector<std::string> panel_type_names(std::uint16_t value) {
    return bitset_names(kPanelTypes, sizeof(kPanelTypes) / sizeof(kPanelTypes[0]), value);
}

std::vector<std::string> panel_flag_names(std::uint8_t value) {
    return bitset_names(kPanelFlags, sizeof(kPanelFlags) / sizeof(kPanelFlags[0]), value);
}

std::vector<std::string> activate_type_names(std::uint8_t value) {
    return bitset_names(kActivateTypes, sizeof(kActivateTypes) / sizeof(kActivateTypes[0]), value);
}

std::vector<std::string> key_names(std::uint8_t value) {
    return bitset_names(kKeys, sizeof(kKeys) / sizeof(kKeys[0]), value);
}

std::vector<std::string> item_option_names(std::uint8_t value) {
    return bitset_names(kItemOptions, sizeof(kItemOptions) / sizeof(kItemOptions[0]), value);
}

} // namespace d2df::tools
