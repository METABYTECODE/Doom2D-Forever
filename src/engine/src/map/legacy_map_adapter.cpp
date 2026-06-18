#include <d2df/engine/map/legacy_map_adapter.hpp>

#include <d2df/engine/map/entity_catalog.hpp>
#include <d2df/map/area_types.hpp>
#include <d2df/map/map_spawn.hpp>

namespace d2df::engine::map {
namespace {

SpawnPoint spawn_from_area(const ::d2df::map::MapArea& area) {
    SpawnPoint spawn;
    switch (area.type) {
    case ::d2df::map::AreaType::PlayerPoint1:
        spawn.id = "player1";
        break;
    case ::d2df::map::AreaType::PlayerPoint2:
        spawn.id = "player2";
        break;
    case ::d2df::map::AreaType::DmPoint:
        spawn.id = "dm";
        break;
    default:
        spawn.id = "unknown";
        break;
    }
    spawn.x = area.position.x;
    spawn.y = area.position.y;
    return spawn;
}

std::string monster_type_name(::d2df::map::MonsterType type) {
    switch (type) {
    case ::d2df::map::MonsterType::Demon:
        return "demon";
    case ::d2df::map::MonsterType::Imp:
        return "imp";
    case ::d2df::map::MonsterType::Zomby:
        return "zomby";
    case ::d2df::map::MonsterType::Serg:
        return "serg";
    case ::d2df::map::MonsterType::Cyber:
        return "cyber";
    case ::d2df::map::MonsterType::Cgun:
        return "cgun";
    case ::d2df::map::MonsterType::Baron:
        return "baron";
    case ::d2df::map::MonsterType::Knight:
        return "knight";
    case ::d2df::map::MonsterType::Caco:
        return "caco";
    case ::d2df::map::MonsterType::Soul:
        return "soul";
    case ::d2df::map::MonsterType::Pain:
        return "pain";
    case ::d2df::map::MonsterType::Spider:
        return "spider";
    case ::d2df::map::MonsterType::Bsp:
        return "bsp";
    case ::d2df::map::MonsterType::Mancub:
        return "mancub";
    case ::d2df::map::MonsterType::Skel:
        return "skel";
    case ::d2df::map::MonsterType::Vile:
        return "vile";
    case ::d2df::map::MonsterType::Fish:
        return "fish";
    case ::d2df::map::MonsterType::Barrel:
        return "barrel";
    case ::d2df::map::MonsterType::Robo:
        return "robo";
    case ::d2df::map::MonsterType::Man:
        return "man";
    default:
        return "none";
    }
}

} // namespace

TileMap tile_map_from_legacy(const ::d2df::map::MapDocument& legacy) {
    TileMap map;
    map.version = 1;
    map.id = legacy.id;
    map.name = legacy.name;
    map.author = legacy.author;
    map.description = legacy.description;
    map.width = legacy.size.width;
    map.height = legacy.size.height;
    map.sky_asset = legacy.sky;
    map.music_asset = legacy.music;

    for (const auto& texture : legacy.textures) {
        TileDef def;
        def.asset_id = texture.path;
        def.animated = texture.animated;
        map.tileset.push_back(def);
    }

    TileLayer layer;
    layer.name = "legacy_panels";
    for (const auto& panel : legacy.panels) {
        TilePlacement tile;
        tile.x = panel.position.x;
        tile.y = panel.position.y;
        tile.width = panel.size.width;
        tile.height = panel.size.height;
        tile.tile_index = panel.texture;
        tile.flags = panel.type;
        tile.visibility = panel.flags;
        tile.alpha = panel.alpha;
        tile.enabled = panel.enabled;
        layer.tiles.push_back(tile);
    }
    map.layers.push_back(std::move(layer));

    for (const auto& area : legacy.areas) {
        if (area.type == ::d2df::map::AreaType::None) {
            continue;
        }
        map.spawns.push_back(spawn_from_area(area));
    }

    for (const auto& monster : legacy.monsters) {
        if (monster.type == ::d2df::map::MonsterType::None) {
            continue;
        }
        MapEntity entity;
        entity.type = monster_type_name(monster.type);
        entity.x = monster.position.x;
        entity.y = monster.position.y;
        const auto dims = entity_dimensions(entity.type);
        if (dims) {
            entity.width = dims->width;
            entity.height = dims->height;
        }
        map.entities.push_back(entity);
    }

    return map;
}

std::optional<SpawnPoint> find_spawn(const TileMap& map, std::string_view spawn_id) {
    for (const auto& spawn : map.spawns) {
        if (spawn.id == spawn_id) {
            return spawn;
        }
    }

    if (spawn_id == "player1") {
        for (const auto& spawn : map.spawns) {
            if (spawn.id == "player2") {
                return spawn;
            }
        }
        for (const auto& spawn : map.spawns) {
            if (spawn.id == "dm") {
                return spawn;
            }
        }
    }

    if (map.width > 0 && map.height > 0) {
        SpawnPoint fallback;
        fallback.id = std::string(spawn_id);
        fallback.x = map.width / 2;
        fallback.y = map.height / 2;
        return fallback;
    }
    return std::nullopt;
}

} // namespace d2df::engine::map
