#include <d2df/sim/game_save.hpp>

#include <fstream>
#include <sstream>

#include <fmt/format.h>

#include <d2df/core/types.hpp>
#include <d2df/ecs/game_world.hpp>
#include <d2df/map/item_types.hpp>
#include <d2df/map/monster_types.hpp>
#include <d2df/sim/item_system.hpp>
#include <d2df/sim/trigger_system.hpp>

namespace d2df::sim {
namespace {

std::string escape_json(const std::string& value) {
    std::string out;
    for (const char c : value) {
        switch (c) {
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out.push_back(c);
            break;
        }
    }
    return out;
}

void capture_combat(const PlayerCombat& combat, PlayerCombatSnapshot& out) {
    out.current_weapon = combat.current_weapon;
    for (std::size_t i = 0; i < kWeaponSlotCount; ++i) {
        out.owned[i] = combat.owned[i];
        out.reloading[i] = combat.reloading[i];
    }
    for (std::size_t i = 0; i < kAmmoTypeCount; ++i) {
        out.ammo[i] = combat.ammo[i];
        out.max_ammo[i] = combat.max_ammo[i];
    }
    out.facing_left = combat.facing_left;
    out.bfg_charge_ticks = combat.bfg_charge_ticks;
    out.bfg_muzzle_x = combat.bfg_muzzle_x;
    out.bfg_muzzle_y = combat.bfg_muzzle_y;
    out.bfg_aim_x = combat.bfg_aim_x;
    out.bfg_aim_y = combat.bfg_aim_y;
}

void restore_combat(PlayerCombat& combat, const PlayerCombatSnapshot& in) {
    combat.current_weapon = in.current_weapon;
    for (std::size_t i = 0; i < kWeaponSlotCount; ++i) {
        combat.owned[i] = in.owned[i];
        combat.reloading[i] = in.reloading[i];
    }
    for (std::size_t i = 0; i < kAmmoTypeCount; ++i) {
        combat.ammo[i] = in.ammo[i];
        combat.max_ammo[i] = in.max_ammo[i];
    }
    combat.facing_left = in.facing_left;
    combat.bfg_charge_ticks = in.bfg_charge_ticks;
    combat.bfg_muzzle_x = in.bfg_muzzle_x;
    combat.bfg_muzzle_y = in.bfg_muzzle_y;
    combat.bfg_aim_x = in.bfg_aim_x;
    combat.bfg_aim_y = in.bfg_aim_y;
}

std::optional<std::string> json_extract_string(std::string_view object, std::string_view key) {
    const std::string needle = fmt::format("\"{}\":", key);
    const auto start = object.find(needle);
    if (start == std::string_view::npos) {
        return std::nullopt;
    }

    std::size_t i = start + needle.size();
    while (i < object.size() && (object[i] == ' ' || object[i] == '\t')) {
        ++i;
    }
    if (i >= object.size() || object[i] != '"') {
        return std::nullopt;
    }

    std::string out;
    bool escape = false;
    for (++i; i < object.size(); ++i) {
        const char c = object[i];
        if (escape) {
            switch (c) {
            case 'n':
                out.push_back('\n');
                break;
            case 'r':
                out.push_back('\r');
                break;
            case 't':
                out.push_back('\t');
                break;
            case '\\':
            case '"':
                out.push_back(c);
                break;
            default:
                out.push_back(c);
                break;
            }
            escape = false;
            continue;
        }
        if (c == '\\') {
            escape = true;
            continue;
        }
        if (c == '"') {
            return out;
        }
        out.push_back(c);
    }
    return std::nullopt;
}

std::optional<std::int64_t> json_extract_number(std::string_view object, std::string_view key) {
    const std::string needle = fmt::format("\"{}\":", key);
    const auto start = object.find(needle);
    if (start == std::string_view::npos) {
        return std::nullopt;
    }
    std::size_t value_start = start + needle.size();
    while (value_start < object.size() && (object[value_start] == ' ' || object[value_start] == '\t')) {
        ++value_start;
    }
    const auto value_end = object.find_first_of(",}\n]", value_start);
    if (value_end == std::string_view::npos) {
        return std::nullopt;
    }
    try {
        return std::stoll(std::string(object.substr(value_start, value_end - value_start)));
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<bool> json_extract_bool(std::string_view object, std::string_view key) {
    const std::string needle = fmt::format("\"{}\":", key);
    const auto start = object.find(needle);
    if (start == std::string_view::npos) {
        return std::nullopt;
    }
    std::size_t value_start = start + needle.size();
    while (value_start < object.size() && (object[value_start] == ' ' || object[value_start] == '\t')) {
        ++value_start;
    }
    if (object.compare(value_start, 4, "true") == 0) {
        return true;
    }
    if (object.compare(value_start, 5, "false") == 0) {
        return false;
    }
    return std::nullopt;
}

std::optional<std::size_t> find_matching_bracket(std::string_view text, std::size_t open_index) {
    if (open_index >= text.size()) {
        return std::nullopt;
    }
    const char open = text[open_index];
    const char close = open == '[' ? ']' : '}';
    int depth = 0;
    for (std::size_t i = open_index; i < text.size(); ++i) {
        if (text[i] == open) {
            ++depth;
        } else if (text[i] == close) {
            --depth;
            if (depth == 0) {
                return i;
            }
        }
    }
    return std::nullopt;
}

std::vector<std::string_view> split_top_level_objects(std::string_view array) {
    std::vector<std::string_view> objects;
    if (array.size() < 2 || array[0] != '[') {
        return objects;
    }

    std::size_t i = 1;
    while (i < array.size()) {
        while (i < array.size() && (array[i] == ' ' || array[i] == '\t' || array[i] == '\n' ||
                                    array[i] == '\r' || array[i] == ',')) {
            ++i;
        }
        if (i >= array.size() || array[i] == ']') {
            break;
        }
        if (array[i] != '{') {
            ++i;
            continue;
        }
        const auto end = find_matching_bracket(array, i);
        if (!end) {
            break;
        }
        objects.emplace_back(array.substr(i, *end - i + 1));
        i = *end + 1;
    }
    return objects;
}

void fill_int_array(std::string_view object, std::string_view key, int* values, std::size_t count) {
    const std::string needle = fmt::format("\"{}\":", key);
    const auto marker = object.find(needle);
    if (marker == std::string_view::npos) {
        return;
    }
    const auto array_start = object.find('[', marker);
    if (array_start == std::string_view::npos) {
        return;
    }
    const auto array_end = object.find(']', array_start);
    if (array_end == std::string_view::npos) {
        return;
    }

    std::size_t index = 0;
    std::size_t i = array_start + 1;
    while (i < array_end && index < count) {
        while (i < array_end && (object[i] == ' ' || object[i] == '\t' || object[i] == ',')) {
            ++i;
        }
        const auto value_end = object.find_first_of(",]", i);
        if (value_end == std::string_view::npos || value_end > array_end) {
            break;
        }
        try {
            values[index] = std::stoi(std::string(object.substr(i, value_end - i)));
        } catch (...) {
            break;
        }
        ++index;
        i = value_end + 1;
    }
}

void fill_bool_array(std::string_view object, std::string_view key, bool* values, std::size_t count) {
    const std::string needle = fmt::format("\"{}\":", key);
    const auto marker = object.find(needle);
    if (marker == std::string_view::npos) {
        return;
    }
    const auto array_start = object.find('[', marker);
    if (array_start == std::string_view::npos) {
        return;
    }
    const auto array_end = object.find(']', array_start);
    if (array_end == std::string_view::npos) {
        return;
    }

    std::size_t index = 0;
    std::size_t i = array_start + 1;
    while (i < array_end && index < count) {
        while (i < array_end && (object[i] == ' ' || object[i] == '\t' || object[i] == ',')) {
            ++i;
        }
        if (object.compare(i, 4, "true") == 0) {
            values[index] = true;
            i += 4;
        } else if (object.compare(i, 5, "false") == 0) {
            values[index] = false;
            i += 5;
        } else {
            break;
        }
        ++index;
    }
}

std::string write_player_json(const PlayerStateSnapshot& player) {
    std::ostringstream json;
    json << "{";
    json << fmt::format("\"x\":{:.4f},\"y\":{:.4f},\"prev_x\":{:.4f},\"prev_y\":{:.4f},", player.x,
                         player.y, player.prev_x, player.prev_y);
    json << fmt::format("\"vel_x\":{},\"vel_y\":{},\"accel_x\":{},\"accel_y\":{},\"tick\":{},",
                         player.vel_x, player.vel_y, player.accel_x, player.accel_y, player.tick);
    json << fmt::format("\"health\":{},\"armor\":{},", player.health, player.armor);
    json << fmt::format("\"key_red\":{},\"key_green\":{},\"key_blue\":{},\"has_backpack\":{},",
                         player.key_red ? "true" : "false", player.key_green ? "true" : "false",
                         player.key_blue ? "true" : "false",
                         player.has_backpack ? "true" : "false");
    json << fmt::format("\"powerup_suit_until\":{},\"powerup_invul_until\":{},",
                         player.powerup_suit_until, player.powerup_invul_until);
    json << fmt::format("\"powerup_invis_until\":{},\"berserk_until\":{},", player.powerup_invis_until,
                         player.berserk_until);
    json << fmt::format("\"jet_fuel\":{},\"air\":{},", player.jet_fuel, player.air);
    json << fmt::format("\"jetpack_active\":{},\"can_jetpack\":{},", player.jetpack_active ? "true"
                                                                                            : "false",
                         player.can_jetpack ? "true" : "false");
    json << fmt::format("\"on_ground\":{},\"in_water\":{},\"in_acid\":{},\"on_lift\":{},",
                         player.on_ground ? "true" : "false", player.in_water ? "true" : "false",
                         player.in_acid ? "true" : "false", player.on_lift ? "true" : "false");
    json << fmt::format("\"run_direction\":{},\"pain_ticks\":{},\"punch_ticks\":{},", player.run_direction,
                         player.pain_ticks, player.punch_ticks);
    json << fmt::format("\"punch_aim_up\":{},\"punch_aim_down\":{},", player.punch_aim_up ? "true"
                                                                                           : "false",
                         player.punch_aim_down ? "true" : "false");
    json << fmt::format("\"death_phase\":{},\"death_started_tick\":{},\"death_health\":{},",
                         static_cast<int>(player.death_phase), player.death_started_tick,
                         player.death_health);
    json << fmt::format("\"corpse_resolved\":{},\"respawn_ticks_remaining\":{},", player.corpse_resolved
                                                                                     ? "true"
                                                                                     : "false",
                         player.respawn_ticks_remaining);
    json << "\"combat\":{";
    const auto& c = player.combat;
    json << fmt::format("\"current_weapon\":{},\"facing_left\":{},\"bfg_charge_ticks\":{},",
                         static_cast<int>(c.current_weapon), c.facing_left ? "true" : "false",
                         c.bfg_charge_ticks);
    json << fmt::format("\"bfg_muzzle_x\":{:.4f},\"bfg_muzzle_y\":{:.4f},\"bfg_aim_x\":{:.4f},",
                         c.bfg_muzzle_x, c.bfg_muzzle_y, c.bfg_aim_x);
    json << fmt::format("\"bfg_aim_y\":{:.4f},\"owned\":[", c.bfg_aim_y);
    for (std::size_t i = 0; i < kWeaponSlotCount; ++i) {
        json << (c.owned[i] ? "true" : "false");
        if (i + 1 < kWeaponSlotCount) {
            json << ",";
        }
    }
    json << "],\"ammo\":[";
    for (std::size_t i = 0; i < kAmmoTypeCount; ++i) {
        json << c.ammo[i];
        if (i + 1 < kAmmoTypeCount) {
            json << ",";
        }
    }
    json << "],\"max_ammo\":[";
    for (std::size_t i = 0; i < kAmmoTypeCount; ++i) {
        json << c.max_ammo[i];
        if (i + 1 < kAmmoTypeCount) {
            json << ",";
        }
    }
    json << "],\"reloading\":[";
    for (std::size_t i = 0; i < kWeaponSlotCount; ++i) {
        json << c.reloading[i];
        if (i + 1 < kWeaponSlotCount) {
            json << ",";
        }
    }
    json << "]}";
    json << "}";
    return json.str();
}

PlayerStateSnapshot parse_player_json(std::string_view object) {
    PlayerStateSnapshot player;
    if (const auto x = json_extract_number(object, "x")) {
        player.x = static_cast<float>(*x);
    }
    if (const auto y = json_extract_number(object, "y")) {
        player.y = static_cast<float>(*y);
    }
    if (const auto prev_x = json_extract_number(object, "prev_x")) {
        player.prev_x = static_cast<float>(*prev_x);
    }
    if (const auto prev_y = json_extract_number(object, "prev_y")) {
        player.prev_y = static_cast<float>(*prev_y);
    }
    if (const auto vel_x = json_extract_number(object, "vel_x")) {
        player.vel_x = static_cast<int>(*vel_x);
    }
    if (const auto vel_y = json_extract_number(object, "vel_y")) {
        player.vel_y = static_cast<int>(*vel_y);
    }
    if (const auto accel_x = json_extract_number(object, "accel_x")) {
        player.accel_x = static_cast<int>(*accel_x);
    }
    if (const auto accel_y = json_extract_number(object, "accel_y")) {
        player.accel_y = static_cast<int>(*accel_y);
    }
    if (const auto tick = json_extract_number(object, "tick")) {
        player.tick = static_cast<int>(*tick);
    }
    if (const auto health = json_extract_number(object, "health")) {
        player.health = static_cast<int>(*health);
    }
    if (const auto armor = json_extract_number(object, "armor")) {
        player.armor = static_cast<int>(*armor);
    }
    if (const auto key_red = json_extract_bool(object, "key_red")) {
        player.key_red = *key_red;
    }
    if (const auto key_green = json_extract_bool(object, "key_green")) {
        player.key_green = *key_green;
    }
    if (const auto key_blue = json_extract_bool(object, "key_blue")) {
        player.key_blue = *key_blue;
    }
    if (const auto has_backpack = json_extract_bool(object, "has_backpack")) {
        player.has_backpack = *has_backpack;
    }
    if (const auto powerup_suit_until = json_extract_number(object, "powerup_suit_until")) {
        player.powerup_suit_until = static_cast<int>(*powerup_suit_until);
    }
    if (const auto powerup_invul_until = json_extract_number(object, "powerup_invul_until")) {
        player.powerup_invul_until = static_cast<int>(*powerup_invul_until);
    }
    if (const auto powerup_invis_until = json_extract_number(object, "powerup_invis_until")) {
        player.powerup_invis_until = static_cast<int>(*powerup_invis_until);
    }
    if (const auto berserk_until = json_extract_number(object, "berserk_until")) {
        player.berserk_until = static_cast<int>(*berserk_until);
    }
    if (const auto jet_fuel = json_extract_number(object, "jet_fuel")) {
        player.jet_fuel = static_cast<int>(*jet_fuel);
    }
    if (const auto air = json_extract_number(object, "air")) {
        player.air = static_cast<int>(*air);
    }
    if (const auto jetpack_active = json_extract_bool(object, "jetpack_active")) {
        player.jetpack_active = *jetpack_active;
    }
    if (const auto can_jetpack = json_extract_bool(object, "can_jetpack")) {
        player.can_jetpack = *can_jetpack;
    }
    if (const auto on_ground = json_extract_bool(object, "on_ground")) {
        player.on_ground = *on_ground;
    }
    if (const auto in_water = json_extract_bool(object, "in_water")) {
        player.in_water = *in_water;
    }
    if (const auto in_acid = json_extract_bool(object, "in_acid")) {
        player.in_acid = *in_acid;
    }
    if (const auto on_lift = json_extract_bool(object, "on_lift")) {
        player.on_lift = *on_lift;
    }
    if (const auto run_direction = json_extract_number(object, "run_direction")) {
        player.run_direction = static_cast<int>(*run_direction);
    }
    if (const auto pain_ticks = json_extract_number(object, "pain_ticks")) {
        player.pain_ticks = static_cast<int>(*pain_ticks);
    }
    if (const auto punch_ticks = json_extract_number(object, "punch_ticks")) {
        player.punch_ticks = static_cast<int>(*punch_ticks);
    }
    if (const auto punch_aim_up = json_extract_bool(object, "punch_aim_up")) {
        player.punch_aim_up = *punch_aim_up;
    }
    if (const auto punch_aim_down = json_extract_bool(object, "punch_aim_down")) {
        player.punch_aim_down = *punch_aim_down;
    }
    if (const auto death_phase = json_extract_number(object, "death_phase")) {
        player.death_phase = static_cast<PlayerDeathPhase>(*death_phase);
    }
    if (const auto death_started_tick = json_extract_number(object, "death_started_tick")) {
        player.death_started_tick = static_cast<int>(*death_started_tick);
    }
    if (const auto death_health = json_extract_number(object, "death_health")) {
        player.death_health = static_cast<int>(*death_health);
    }
    if (const auto corpse_resolved = json_extract_bool(object, "corpse_resolved")) {
        player.corpse_resolved = *corpse_resolved;
    }
    if (const auto respawn_ticks_remaining = json_extract_number(object, "respawn_ticks_remaining")) {
        player.respawn_ticks_remaining = static_cast<int>(*respawn_ticks_remaining);
    }

    const auto combat_start = object.find("\"combat\":");
    if (combat_start != std::string_view::npos) {
        const auto brace = object.find('{', combat_start);
        if (brace != std::string_view::npos) {
            const auto combat_end = find_matching_bracket(object, brace);
            if (combat_end) {
                const auto combat = object.substr(brace, *combat_end - brace + 1);
                auto& c = player.combat;
                if (const auto current_weapon = json_extract_number(combat, "current_weapon")) {
                    c.current_weapon = static_cast<WeaponId>(*current_weapon);
                }
                if (const auto facing_left = json_extract_bool(combat, "facing_left")) {
                    c.facing_left = *facing_left;
                }
                if (const auto bfg_charge_ticks = json_extract_number(combat, "bfg_charge_ticks")) {
                    c.bfg_charge_ticks = static_cast<int>(*bfg_charge_ticks);
                }
                if (const auto bfg_muzzle_x = json_extract_number(combat, "bfg_muzzle_x")) {
                    c.bfg_muzzle_x = static_cast<float>(*bfg_muzzle_x);
                }
                if (const auto bfg_muzzle_y = json_extract_number(combat, "bfg_muzzle_y")) {
                    c.bfg_muzzle_y = static_cast<float>(*bfg_muzzle_y);
                }
                if (const auto bfg_aim_x = json_extract_number(combat, "bfg_aim_x")) {
                    c.bfg_aim_x = static_cast<float>(*bfg_aim_x);
                }
                if (const auto bfg_aim_y = json_extract_number(combat, "bfg_aim_y")) {
                    c.bfg_aim_y = static_cast<float>(*bfg_aim_y);
                }
                fill_bool_array(combat, "owned", c.owned, kWeaponSlotCount);
                fill_int_array(combat, "ammo", c.ammo, kAmmoTypeCount);
                fill_int_array(combat, "max_ammo", c.max_ammo, kAmmoTypeCount);
                fill_int_array(combat, "reloading", c.reloading, kWeaponSlotCount);
            }
        }
    }
    return player;
}

} // namespace

void capture_player_state(const PlayerState& player, PlayerStateSnapshot& out) {
    out.x = player.x;
    out.y = player.y;
    out.prev_x = player.prev_x;
    out.prev_y = player.prev_y;
    out.vel_x = player.vel_x;
    out.vel_y = player.vel_y;
    out.accel_x = player.accel_x;
    out.accel_y = player.accel_y;
    out.tick = player.tick();
    out.health = player.health();
    out.armor = player.armor();
    out.key_red = player.has_key_red();
    out.key_green = player.has_key_green();
    out.key_blue = player.has_key_blue();
    out.has_backpack = player.has_backpack();
    out.powerup_suit_until = player.tick() + player.powerup_suit_ticks_remaining();
    out.powerup_invul_until = player.tick() + player.powerup_invul_ticks_remaining();
    out.powerup_invis_until = player.tick() + player.powerup_invis_ticks_remaining();
    out.berserk_until = player.tick() + player.berserk_ticks_remaining();
    out.jet_fuel = player.jet_fuel();
    out.air = player.air();
    out.jetpack_active = player.jetpack_active();
    out.can_jetpack = false;
    out.on_ground = player.on_ground();
    out.in_water = player.in_water();
    out.in_acid = player.in_acid();
    out.on_lift = player.on_lift();
    out.run_direction = 0;
    out.pain_ticks = player.pain_ticks();
    out.punch_ticks = player.punch_ticks();
    out.punch_aim_up = player.punch_aim_up();
    out.punch_aim_down = player.punch_aim_down();
    out.death_phase = player.death_phase();
    out.death_started_tick = player.death_started_tick();
    out.death_health = player.death_health();
    out.corpse_resolved = player.corpse_resolved();
    out.respawn_ticks_remaining = player.respawn_ticks_remaining();
    capture_combat(player.combat(), out.combat);
}

void restore_player_state(PlayerState& player, const PlayerStateSnapshot& in) {
    player.snap_to(in.x, in.y);
    player.x = in.x;
    player.y = in.y;
    player.prev_x = in.prev_x;
    player.prev_y = in.prev_y;
    player.vel_x = in.vel_x;
    player.vel_y = in.vel_y;
    player.accel_x = in.accel_x;
    player.accel_y = in.accel_y;

    player.set_health(in.health);
    player.set_armor(in.armor);
    restore_combat(player.combat(), in.combat);
    player.restore_runtime_snapshot(in);
}

void ItemSystem::export_save_state(std::vector<WorldItemSnapshot>& out) const {
    out.clear();
    for (const auto& item : items_) {
        WorldItemSnapshot snap;
        snap.type = static_cast<std::uint8_t>(item.type);
        snap.x = item.x;
        snap.y = item.y;
        snap.spawn_x = item.spawn_x;
        snap.spawn_y = item.spawn_y;
        snap.width = item.width;
        snap.height = item.height;
        snap.active = item.active;
        snap.respawnable = item.respawnable;
        snap.fall = item.fall;
        snap.dropped = item.dropped;
        snap.vel_x = item.vel_x;
        snap.vel_y = item.vel_y;
        snap.anim_tick = item.anim_tick;
        snap.respawn_anim_tick = item.respawn_anim_tick;
        snap.respawn_countdown = item.respawn_countdown;
        out.push_back(snap);
    }
}

void ItemSystem::import_save_state(const std::vector<WorldItemSnapshot>& in) {
    items_.clear();
    for (const auto& snap : in) {
        WorldItem item;
        item.type = static_cast<map::ItemType>(snap.type);
        item.x = snap.x;
        item.y = snap.y;
        item.spawn_x = snap.spawn_x;
        item.spawn_y = snap.spawn_y;
        item.width = snap.width;
        item.height = snap.height;
        item.active = snap.active;
        item.respawnable = snap.respawnable;
        item.fall = snap.fall;
        item.dropped = snap.dropped;
        item.vel_x = snap.vel_x;
        item.vel_y = snap.vel_y;
        item.anim_tick = snap.anim_tick;
        item.respawn_anim_tick = snap.respawn_anim_tick;
        item.respawn_countdown = snap.respawn_countdown;
        items_.push_back(item);
    }
}

void TriggerSystem::export_save_state(GameSaveDocument& out) const {
    out.panels.clear();
    for (const auto& panel : panels_) {
        PanelSnapshot snap;
        snap.x = panel.position.x;
        snap.y = panel.position.y;
        snap.width = panel.size.width;
        snap.height = panel.size.height;
        snap.texture = panel.texture;
        snap.type = panel.type;
        snap.alpha = panel.alpha;
        snap.flags = panel.flags;
        snap.enabled = panel.enabled;
        out.panels.push_back(snap);
    }

    out.triggers.clear();
    for (const auto& trigger : triggers_) {
        TriggerRuntimeSnapshot snap;
        snap.enabled = trigger.enabled;
        out.triggers.push_back(snap);
    }
    for (std::size_t i = 0; i < trigger_timeout_.size(); ++i) {
        if (i < out.triggers.size()) {
            out.triggers[i].timeout = trigger_timeout_[i];
        }
    }

    out.trap_timers.clear();
    for (const auto& timer : trap_timers_) {
        out.trap_timers.push_back({timer.first, timer.second});
    }
    out.door5_timers.clear();
    for (const auto& timer : door5_timers_) {
        out.door5_timers.push_back({timer.first, timer.second});
    }

    out.expanders.clear();
    for (const auto& expander : expander_) {
        ExpanderSnapshot snap;
        snap.press_count = expander.press_count;
        snap.press_time = expander.press_time;
        out.expanders.push_back(snap);
    }

    out.nomonster_boot_done = nomonster_boot_done_;
}

void TriggerSystem::import_save_state(const GameSaveDocument& in) {
    if (in.panels.size() == panels_.size()) {
        for (std::size_t i = 0; i < panels_.size(); ++i) {
            const auto& snap = in.panels[i];
            panels_[i].position.x = snap.x;
            panels_[i].position.y = snap.y;
            panels_[i].size.width = snap.width;
            panels_[i].size.height = snap.height;
            panels_[i].texture = snap.texture;
            panels_[i].type = snap.type;
            panels_[i].alpha = snap.alpha;
            panels_[i].flags = snap.flags;
            panels_[i].enabled = snap.enabled;
        }
    }

    if (in.triggers.size() == triggers_.size()) {
        for (std::size_t i = 0; i < triggers_.size(); ++i) {
            triggers_[i].enabled = in.triggers[i].enabled;
        }
    }

    trigger_timeout_.assign(triggers_.size(), 0);
    for (std::size_t i = 0; i < in.triggers.size() && i < trigger_timeout_.size(); ++i) {
        trigger_timeout_[i] = in.triggers[i].timeout;
    }

    trap_timers_.clear();
    for (const auto& timer : in.trap_timers) {
        trap_timers_.emplace_back(timer.panel_index, timer.ticks_remaining);
    }
    door5_timers_.clear();
    for (const auto& timer : in.door5_timers) {
        door5_timers_.emplace_back(timer.panel_index, timer.ticks_remaining);
    }

    expander_.assign(triggers_.size(), {});
    for (std::size_t i = 0; i < in.expanders.size() && i < expander_.size(); ++i) {
        expander_[i].press_count = in.expanders[i].press_count;
        expander_[i].press_time = in.expanders[i].press_time;
    }

    nomonster_boot_done_.assign(triggers_.size(), false);
    for (std::size_t i = 0; i < in.nomonster_boot_done.size() && i < nomonster_boot_done_.size();
         ++i) {
        nomonster_boot_done_[i] = in.nomonster_boot_done[i];
    }

    exit_requested_ = false;
}

void capture_world_save(const ecs::GameWorld& world, const TriggerSystem& triggers,
                        const std::string& map_rel_path, const std::string& map_id,
                        GameSaveDocument& out) {
    out.version = kGameSaveVersion;
    out.map_path = map_rel_path;
    out.map_id = map_id;
    capture_player_state(world.player(), out.player);

    out.monsters.clear();
    for (const auto& target : world.targets()) {
        MonsterSnapshot snap;
        snap.id = target.id;
        snap.monster_type = static_cast<std::uint8_t>(target.monster_type);
        snap.x = target.x;
        snap.y = target.y;
        snap.prev_x = target.prev_x;
        snap.prev_y = target.prev_y;
        snap.width = target.width;
        snap.height = target.height;
        snap.vel_x = target.vel_x;
        snap.vel_y = target.vel_y;
        snap.health = target.health;
        snap.max_health = target.max_health;
        snap.melee_cooldown = target.melee_cooldown;
        snap.shoot_cooldown = target.shoot_cooldown;
        snap.anim_tick = target.anim_tick;
        snap.revive_ticks = target.revive_ticks;
        snap.facing_left = target.facing_left;
        snap.aggro_player = target.aggro_player;
        snap.is_awake = target.is_awake;
        snap.in_water = target.in_water;
        snap.death_handled = target.death_handled;
        snap.life_state = target.life_state;
        out.monsters.push_back(snap);
    }

    world.items().export_save_state(out.items);
    triggers.export_save_state(out);
}

void restore_world_save(ecs::GameWorld& world, TriggerSystem& triggers, const GameSaveDocument& in) {
    restore_player_state(world.player(), in.player);

    auto& targets = world.targets();
    targets.clear();
    EntityId max_id = kDebugTargetBaseId;
    for (const auto& snap : in.monsters) {
        ShootableTarget target;
        target.id = snap.id;
        target.monster_type = static_cast<map::MonsterType>(snap.monster_type);
        target.x = snap.x;
        target.y = snap.y;
        target.prev_x = snap.prev_x;
        target.prev_y = snap.prev_y;
        target.width = snap.width;
        target.height = snap.height;
        target.vel_x = snap.vel_x;
        target.vel_y = snap.vel_y;
        target.health = snap.health;
        target.max_health = snap.max_health;
        target.melee_cooldown = snap.melee_cooldown;
        target.shoot_cooldown = snap.shoot_cooldown;
        target.anim_tick = snap.anim_tick;
        target.revive_ticks = snap.revive_ticks;
        target.facing_left = snap.facing_left;
        target.aggro_player = snap.aggro_player;
        target.is_awake = snap.is_awake;
        target.in_water = snap.in_water;
        target.death_handled = snap.death_handled;
        target.life_state = snap.life_state;
        targets.push_back(target);
        if (target.id >= max_id) {
            max_id = target.id + 1;
        }
    }

    world.items().import_save_state(in.items);
    triggers.import_save_state(in);
    world.projectiles().clear();
    world.player_corpses().clear();
}

bool write_game_save(const std::filesystem::path& path, const GameSaveDocument& doc,
                     std::string& error) {
    std::ostringstream json;
    json << "{\n";
    json << fmt::format("  \"version\": {},\n", doc.version);
    json << fmt::format("  \"map_path\": \"{}\",\n", escape_json(doc.map_path));
    json << fmt::format("  \"map_id\": \"{}\",\n", escape_json(doc.map_id));
    json << fmt::format("  \"player\": {},\n", write_player_json(doc.player));

    json << "  \"monsters\": [\n";
    for (std::size_t i = 0; i < doc.monsters.size(); ++i) {
        const auto& m = doc.monsters[i];
        json << fmt::format(
            "    {{\"id\":{},\"type\":{},\"x\":{:.4f},\"y\":{:.4f},\"prev_x\":{:.4f},\"prev_y\":{:.4f},\"width\":{:.4f},\"height\":{:.4f},\"vel_x\":{},\"vel_y\":{},\"health\":{},\"max_health\":{},\"melee_cd\":{},\"shoot_cd\":{},\"anim_tick\":{},\"revive_ticks\":{},\"facing_left\":{},\"aggro\":{},\"awake\":{},\"in_water\":{},\"death_handled\":{},\"life_state\":{}}}{}",
            m.id, m.monster_type, m.x, m.y, m.prev_x, m.prev_y, m.width, m.height, m.vel_x,
            m.vel_y, m.health, m.max_health, m.melee_cooldown, m.shoot_cooldown, m.anim_tick,
            m.revive_ticks, m.facing_left ? "true" : "false", m.aggro_player ? "true" : "false",
            m.is_awake ? "true" : "false", m.in_water ? "true" : "false",
            m.death_handled ? "true" : "false", static_cast<int>(m.life_state),
            i + 1 < doc.monsters.size() ? "," : "");
        json << "\n";
    }
    json << "  ],\n";

    json << "  \"items\": [\n";
    for (std::size_t i = 0; i < doc.items.size(); ++i) {
        const auto& item = doc.items[i];
        json << fmt::format(
            "    {{\"type\":{},\"x\":{:.4f},\"y\":{:.4f},\"spawn_x\":{:.4f},\"spawn_y\":{:.4f},\"width\":{:.4f},\"height\":{:.4f},\"active\":{},\"respawnable\":{},\"fall\":{},\"dropped\":{},\"vel_x\":{},\"vel_y\":{},\"anim_tick\":{},\"respawn_anim_tick\":{},\"respawn_countdown\":{}}}{}",
            item.type, item.x, item.y, item.spawn_x, item.spawn_y, item.width, item.height,
            item.active ? "true" : "false", item.respawnable ? "true" : "false",
            item.fall ? "true" : "false", item.dropped ? "true" : "false", item.vel_x, item.vel_y,
            item.anim_tick, item.respawn_anim_tick, item.respawn_countdown,
            i + 1 < doc.items.size() ? "," : "");
        json << "\n";
    }
    json << "  ],\n";

    json << "  \"panels\": [\n";
    for (std::size_t i = 0; i < doc.panels.size(); ++i) {
        const auto& panel = doc.panels[i];
        json << fmt::format(
            "    {{\"x\":{},\"y\":{},\"width\":{},\"height\":{},\"texture\":{},\"type\":{},\"alpha\":{},\"flags\":{},\"enabled\":{}}}{}",
            panel.x, panel.y, panel.width, panel.height, panel.texture, panel.type, panel.alpha,
            panel.flags, panel.enabled ? "true" : "false", i + 1 < doc.panels.size() ? "," : "");
        json << "\n";
    }
    json << "  ]\n";
    json << "}\n";

    std::ofstream out(path, std::ios::binary);
    if (!out) {
        error = "failed to open save file for writing";
        return false;
    }
    out << json.str();
    if (!out) {
        error = "failed to write save file";
        return false;
    }
    return true;
}

bool read_game_save(const std::filesystem::path& path, GameSaveDocument& doc, std::string& error) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        error = "save file not found";
        return false;
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    const std::string json = buffer.str();

    if (const auto version = json_extract_number(json, "version")) {
        doc.version = static_cast<int>(*version);
    }
    if (doc.version != kGameSaveVersion) {
        error = "unsupported save version";
        return false;
    }
    if (const auto map_path = json_extract_string(json, "map_path")) {
        doc.map_path = *map_path;
    }
    if (const auto map_id = json_extract_string(json, "map_id")) {
        doc.map_id = *map_id;
    }

    const auto player_start = json.find("\"player\":");
    if (player_start != std::string::npos) {
        const auto brace = json.find('{', player_start);
        if (brace != std::string::npos) {
            const auto end = find_matching_bracket(json, brace);
            if (end) {
                doc.player = parse_player_json(json.substr(brace, *end - brace + 1));
            }
        }
    }

    doc.monsters.clear();
    const auto monsters_marker = json.find("\"monsters\":");
    if (monsters_marker != std::string::npos) {
        const auto array_start = json.find('[', monsters_marker);
        if (array_start != std::string::npos) {
            const auto array_end = find_matching_bracket(json, array_start);
            if (array_end) {
                const auto objects =
                    split_top_level_objects(json.substr(array_start, *array_end - array_start + 1));
                for (const auto object : objects) {
                    MonsterSnapshot snap;
                    if (const auto id = json_extract_number(object, "id")) {
                        snap.id = static_cast<EntityId>(*id);
                    }
                    if (const auto type = json_extract_number(object, "type")) {
                        snap.monster_type = static_cast<std::uint8_t>(*type);
                    }
                    if (const auto x = json_extract_number(object, "x")) {
                        snap.x = static_cast<float>(*x);
                    }
                    if (const auto y = json_extract_number(object, "y")) {
                        snap.y = static_cast<float>(*y);
                    }
                    if (const auto prev_x = json_extract_number(object, "prev_x")) {
                        snap.prev_x = static_cast<float>(*prev_x);
                    }
                    if (const auto prev_y = json_extract_number(object, "prev_y")) {
                        snap.prev_y = static_cast<float>(*prev_y);
                    }
                    if (const auto width = json_extract_number(object, "width")) {
                        snap.width = static_cast<float>(*width);
                    }
                    if (const auto height = json_extract_number(object, "height")) {
                        snap.height = static_cast<float>(*height);
                    }
                    if (const auto vel_x = json_extract_number(object, "vel_x")) {
                        snap.vel_x = static_cast<int>(*vel_x);
                    }
                    if (const auto vel_y = json_extract_number(object, "vel_y")) {
                        snap.vel_y = static_cast<int>(*vel_y);
                    }
                    if (const auto health = json_extract_number(object, "health")) {
                        snap.health = static_cast<int>(*health);
                    }
                    if (const auto max_health = json_extract_number(object, "max_health")) {
                        snap.max_health = static_cast<int>(*max_health);
                    }
                    if (const auto melee_cd = json_extract_number(object, "melee_cd")) {
                        snap.melee_cooldown = static_cast<int>(*melee_cd);
                    }
                    if (const auto shoot_cd = json_extract_number(object, "shoot_cd")) {
                        snap.shoot_cooldown = static_cast<int>(*shoot_cd);
                    }
                    if (const auto anim_tick = json_extract_number(object, "anim_tick")) {
                        snap.anim_tick = static_cast<int>(*anim_tick);
                    }
                    if (const auto revive_ticks = json_extract_number(object, "revive_ticks")) {
                        snap.revive_ticks = static_cast<int>(*revive_ticks);
                    }
                    if (const auto facing_left = json_extract_bool(object, "facing_left")) {
                        snap.facing_left = *facing_left;
                    }
                    if (const auto aggro = json_extract_bool(object, "aggro")) {
                        snap.aggro_player = *aggro;
                    }
                    if (const auto awake = json_extract_bool(object, "awake")) {
                        snap.is_awake = *awake;
                    }
                    if (const auto in_water = json_extract_bool(object, "in_water")) {
                        snap.in_water = *in_water;
                    }
                    if (const auto death_handled = json_extract_bool(object, "death_handled")) {
                        snap.death_handled = *death_handled;
                    }
                    if (const auto life_state = json_extract_number(object, "life_state")) {
                        snap.life_state = static_cast<MonsterLifeState>(*life_state);
                    }
                    doc.monsters.push_back(snap);
                }
            }
        }
    }

    doc.items.clear();
    const auto items_marker = json.find("\"items\":");
    if (items_marker != std::string::npos) {
        const auto array_start = json.find('[', items_marker);
        if (array_start != std::string::npos) {
            const auto array_end = find_matching_bracket(json, array_start);
            if (array_end) {
                const auto objects =
                    split_top_level_objects(json.substr(array_start, *array_end - array_start + 1));
                for (const auto object : objects) {
                    WorldItemSnapshot snap;
                    if (const auto type = json_extract_number(object, "type")) {
                        snap.type = static_cast<std::uint8_t>(*type);
                    }
                    if (const auto x = json_extract_number(object, "x")) {
                        snap.x = static_cast<float>(*x);
                    }
                    if (const auto y = json_extract_number(object, "y")) {
                        snap.y = static_cast<float>(*y);
                    }
                    if (const auto spawn_x = json_extract_number(object, "spawn_x")) {
                        snap.spawn_x = static_cast<float>(*spawn_x);
                    }
                    if (const auto spawn_y = json_extract_number(object, "spawn_y")) {
                        snap.spawn_y = static_cast<float>(*spawn_y);
                    }
                    if (const auto width = json_extract_number(object, "width")) {
                        snap.width = static_cast<float>(*width);
                    }
                    if (const auto height = json_extract_number(object, "height")) {
                        snap.height = static_cast<float>(*height);
                    }
                    if (const auto active = json_extract_bool(object, "active")) {
                        snap.active = *active;
                    }
                    if (const auto respawnable = json_extract_bool(object, "respawnable")) {
                        snap.respawnable = *respawnable;
                    }
                    if (const auto fall = json_extract_bool(object, "fall")) {
                        snap.fall = *fall;
                    }
                    if (const auto dropped = json_extract_bool(object, "dropped")) {
                        snap.dropped = *dropped;
                    }
                    if (const auto vel_x = json_extract_number(object, "vel_x")) {
                        snap.vel_x = static_cast<int>(*vel_x);
                    }
                    if (const auto vel_y = json_extract_number(object, "vel_y")) {
                        snap.vel_y = static_cast<int>(*vel_y);
                    }
                    if (const auto anim_tick = json_extract_number(object, "anim_tick")) {
                        snap.anim_tick = static_cast<int>(*anim_tick);
                    }
                    if (const auto respawn_anim_tick =
                            json_extract_number(object, "respawn_anim_tick")) {
                        snap.respawn_anim_tick = static_cast<int>(*respawn_anim_tick);
                    }
                    if (const auto respawn_countdown =
                            json_extract_number(object, "respawn_countdown")) {
                        snap.respawn_countdown = static_cast<int>(*respawn_countdown);
                    }
                    doc.items.push_back(snap);
                }
            }
        }
    }

    doc.panels.clear();
    const auto panels_marker = json.find("\"panels\":");
    if (panels_marker != std::string::npos) {
        const auto array_start = json.find('[', panels_marker);
        if (array_start != std::string::npos) {
            const auto array_end = find_matching_bracket(json, array_start);
            if (array_end) {
                const auto objects =
                    split_top_level_objects(json.substr(array_start, *array_end - array_start + 1));
                for (const auto object : objects) {
                    PanelSnapshot snap;
                    if (const auto x = json_extract_number(object, "x")) {
                        snap.x = static_cast<std::int32_t>(*x);
                    }
                    if (const auto y = json_extract_number(object, "y")) {
                        snap.y = static_cast<std::int32_t>(*y);
                    }
                    if (const auto width = json_extract_number(object, "width")) {
                        snap.width = static_cast<std::int32_t>(*width);
                    }
                    if (const auto height = json_extract_number(object, "height")) {
                        snap.height = static_cast<std::int32_t>(*height);
                    }
                    if (const auto texture = json_extract_number(object, "texture")) {
                        snap.texture = static_cast<std::uint16_t>(*texture);
                    }
                    if (const auto type = json_extract_number(object, "type")) {
                        snap.type = static_cast<std::uint16_t>(*type);
                    }
                    if (const auto alpha = json_extract_number(object, "alpha")) {
                        snap.alpha = static_cast<std::uint8_t>(*alpha);
                    }
                    if (const auto flags = json_extract_number(object, "flags")) {
                        snap.flags = static_cast<std::uint8_t>(*flags);
                    }
                    if (const auto enabled = json_extract_bool(object, "enabled")) {
                        snap.enabled = *enabled;
                    }
                    doc.panels.push_back(snap);
                }
            }
        }
    }

    return true;
}

} // namespace d2df::sim
