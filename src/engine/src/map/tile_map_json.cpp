#include <d2df/engine/map/tile_map_json.hpp>

#include <fstream>
#include <stdexcept>
#include <string_view>

#include <d2df/engine/map/entity_catalog.hpp>
#include <d2df/engine/map/tile_flags.hpp>

namespace d2df::engine::map {
namespace {

std::optional<std::string> json_extract_string(std::string_view object, std::string_view key) {
    const std::string needle = std::string("\"") + std::string(key) + "\":";
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

std::optional<std::int32_t> json_extract_int(std::string_view object, std::string_view key) {
    const std::string needle = std::string("\"") + std::string(key) + "\":";
    const auto start = object.find(needle);
    if (start == std::string_view::npos) {
        return std::nullopt;
    }
    std::size_t value_start = start + needle.size();
    while (value_start < object.size() && (object[value_start] == ' ' || object[value_start] == '\t')) {
        ++value_start;
    }
    const auto value_end = object.find_first_of(",}\n", value_start);
    if (value_end == std::string_view::npos) {
        return std::nullopt;
    }
    return std::stoi(std::string(object.substr(value_start, value_end - value_start)));
}

std::optional<bool> json_extract_bool(std::string_view object, std::string_view key) {
    const std::string needle = std::string("\"") + std::string(key) + "\":";
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

std::optional<std::size_t> find_matching_brace(std::string_view text, std::size_t open_pos) {
    if (open_pos >= text.size() || text[open_pos] != '{') {
        return std::nullopt;
    }

    int depth = 0;
    bool in_string = false;
    bool escape = false;
    for (std::size_t i = open_pos; i < text.size(); ++i) {
        const char c = text[i];
        if (in_string) {
            if (escape) {
                escape = false;
            } else if (c == '\\') {
                escape = true;
            } else if (c == '"') {
                in_string = false;
            }
            continue;
        }

        if (c == '"') {
            in_string = true;
            continue;
        }
        if (c == '{') {
            ++depth;
        } else if (c == '}') {
            --depth;
            if (depth == 0) {
                return i;
            }
        }
    }
    return std::nullopt;
}

std::optional<std::size_t> find_matching_bracket(std::string_view text, std::size_t open_pos) {
    if (open_pos >= text.size() || text[open_pos] != '[') {
        return std::nullopt;
    }

    int depth = 0;
    bool in_string = false;
    bool escape = false;
    for (std::size_t i = open_pos; i < text.size(); ++i) {
        const char c = text[i];
        if (in_string) {
            if (escape) {
                escape = false;
            } else if (c == '\\') {
                escape = true;
            } else if (c == '"') {
                in_string = false;
            }
            continue;
        }

        if (c == '"') {
            in_string = true;
            continue;
        }
        if (c == '[') {
            ++depth;
        } else if (c == ']') {
            --depth;
            if (depth == 0) {
                return i;
            }
        }
    }
    return std::nullopt;
}

std::uint32_t flag_from_name(std::string_view name) {
    if (name == "wall") {
        return TILE_WALL;
    }
    if (name == "back") {
        return TILE_BACK;
    }
    if (name == "fore") {
        return TILE_FORE;
    }
    if (name == "water") {
        return TILE_WATER;
    }
    if (name == "acid1") {
        return TILE_ACID1;
    }
    if (name == "acid2") {
        return TILE_ACID2;
    }
    if (name == "step") {
        return TILE_STEP;
    }
    if (name == "lift_up") {
        return TILE_LIFT_UP;
    }
    if (name == "lift_down") {
        return TILE_LIFT_DOWN;
    }
    if (name == "open_door") {
        return TILE_OPEN_DOOR;
    }
    if (name == "close_door") {
        return TILE_CLOSE_DOOR;
    }
    if (name == "block_mon") {
        return TILE_BLOCK_MON;
    }
    if (name == "lift_left") {
        return TILE_LIFT_LEFT;
    }
    if (name == "lift_right") {
        return TILE_LIFT_RIGHT;
    }
    return 0;
}

std::uint32_t parse_flags(std::string_view object) {
    const std::string marker = "\"flags\":";
    const auto start = object.find(marker);
    if (start == std::string_view::npos) {
        return 0;
    }

    std::size_t value_start = start + marker.size();
    while (value_start < object.size() && (object[value_start] == ' ' || object[value_start] == '\t')) {
        ++value_start;
    }
    if (value_start < object.size() && object[value_start] != '[') {
        const auto value_end = object.find_first_of(",}\n", value_start);
        if (value_end != std::string_view::npos) {
            try {
                return static_cast<std::uint32_t>(
                    std::stoi(std::string(object.substr(value_start, value_end - value_start))));
            } catch (...) {
                // fall through to string-array parsing
            }
        }
    }

    const auto array_start = object.find('[', start);
    if (array_start == std::string_view::npos) {
        return 0;
    }

    const auto array_end = find_matching_bracket(object, array_start);
    if (!array_end) {
        return 0;
    }

    std::uint32_t bits = 0;
    const auto array = object.substr(array_start, *array_end - array_start + 1);
    std::size_t pos = 0;
    while ((pos = array.find('"', pos)) != std::string_view::npos) {
        const auto name_start = pos + 1;
        const auto name_end = array.find('"', name_start);
        if (name_end == std::string_view::npos) {
            break;
        }
        bits |= flag_from_name(array.substr(name_start, name_end - name_start));
        pos = name_end + 1;
    }
    return bits;
}

std::vector<TileDef> parse_tileset(std::string_view json) {
    std::vector<TileDef> tileset;
    const auto marker = "\"tileset\":";
    const auto start = json.find(marker);
    if (start == std::string_view::npos) {
        return tileset;
    }

    const auto array_start = json.find('[', start);
    if (array_start == std::string_view::npos) {
        return tileset;
    }

    const auto array_end = find_matching_bracket(json, array_start);
    if (!array_end) {
        return tileset;
    }

    std::size_t pos = array_start;
    while (true) {
        const auto object_start = json.find('{', pos);
        if (object_start == std::string_view::npos || object_start > *array_end) {
            break;
        }
        const auto object_end = find_matching_brace(json, object_start);
        if (!object_end || *object_end > *array_end) {
            break;
        }

        const auto object = json.substr(object_start, *object_end - object_start + 1);
        TileDef def;
        def.asset_id = json_extract_string(object, "asset").value_or(
            json_extract_string(object, "path").value_or(""));
        def.width = json_extract_int(object, "width").value_or(
            json_extract_int(object, "w").value_or(0));
        def.height = json_extract_int(object, "height").value_or(
            json_extract_int(object, "h").value_or(0));
        def.animated = json_extract_bool(object, "animated").value_or(false);
        tileset.push_back(def);

        pos = *object_end + 1;
    }

    return tileset;
}

TilePlacement parse_tile(std::string_view object) {
    TilePlacement tile;
    tile.x = json_extract_int(object, "x").value_or(0);
    tile.y = json_extract_int(object, "y").value_or(0);
    tile.width = json_extract_int(object, "w").value_or(json_extract_int(object, "width").value_or(0));
    tile.height = json_extract_int(object, "h").value_or(json_extract_int(object, "height").value_or(0));
    tile.tile_index =
        static_cast<std::uint16_t>(json_extract_int(object, "tile").value_or(0));
    tile.flags = parse_flags(object);
    tile.alpha = static_cast<std::uint8_t>(json_extract_int(object, "alpha").value_or(0));
  tile.enabled = json_extract_bool(object, "enabled").value_or(true);

    const bool visible = json_extract_bool(object, "visible").value_or(true);
    if (!visible) {
        tile.visibility |= TILE_VIS_HIDDEN;
    }
    const bool blend = json_extract_bool(object, "blend").value_or(false);
    if (blend) {
        tile.visibility |= TILE_VIS_BLEND;
    }

    return tile;
}

std::vector<TileLayer> parse_layers(std::string_view json) {
    std::vector<TileLayer> layers;
    const auto marker = "\"layers\":";
    const auto start = json.find(marker);
    if (start == std::string_view::npos) {
        return layers;
    }

    const auto array_start = json.find('[', start);
    if (array_start == std::string_view::npos) {
        return layers;
    }

    const auto array_end = find_matching_bracket(json, array_start);
    if (!array_end) {
        return layers;
    }

    std::size_t pos = array_start;
    while (true) {
        const auto layer_start = json.find('{', pos);
        if (layer_start == std::string_view::npos || layer_start > *array_end) {
            break;
        }
        const auto layer_end = find_matching_brace(json, layer_start);
        if (!layer_end || *layer_end > *array_end) {
            break;
        }

        const auto layer_object = json.substr(layer_start, *layer_end - layer_start + 1);
        TileLayer layer;
        layer.name = json_extract_string(layer_object, "name").value_or("layer");

        const auto tiles_marker = "\"tiles\":";
        const auto tiles_start = layer_object.find(tiles_marker);
        if (tiles_start != std::string_view::npos) {
            const auto tile_array_start = layer_object.find('[', tiles_start);
            if (tile_array_start != std::string_view::npos) {
                const auto tile_array_end = find_matching_bracket(layer_object, tile_array_start);
                if (tile_array_end) {
                    std::size_t tile_pos = tile_array_start;
                    while (true) {
                        const auto tile_start = layer_object.find('{', tile_pos);
                        if (tile_start == std::string_view::npos || tile_start > *tile_array_end) {
                            break;
                        }
                        const auto tile_end = find_matching_brace(layer_object, tile_start);
                        if (!tile_end || *tile_end > *tile_array_end) {
                            break;
                        }
                        const auto tile_object =
                            layer_object.substr(tile_start, *tile_end - tile_start + 1);
                        layer.tiles.push_back(parse_tile(tile_object));
                        tile_pos = *tile_end + 1;
                    }
                }
            }
        }

        layers.push_back(std::move(layer));
        pos = *layer_end + 1;
    }

    return layers;
}

std::vector<SpawnPoint> parse_spawns(std::string_view json) {
    std::vector<SpawnPoint> spawns;
    const auto marker = "\"spawns\":";
    const auto start = json.find(marker);
    if (start == std::string_view::npos) {
        return spawns;
    }

    const auto array_start = json.find('[', start);
    if (array_start == std::string_view::npos) {
        return spawns;
    }

    const auto array_end = find_matching_bracket(json, array_start);
    if (!array_end) {
        return spawns;
    }

    std::size_t pos = array_start;
    while (true) {
        const auto object_start = json.find('{', pos);
        if (object_start == std::string_view::npos || object_start > *array_end) {
            break;
        }
        const auto object_end = find_matching_brace(json, object_start);
        if (!object_end || *object_end > *array_end) {
            break;
        }

        const auto object = json.substr(object_start, *object_end - object_start + 1);
        SpawnPoint spawn;
        spawn.id = json_extract_string(object, "id").value_or("player1");
        spawn.x = json_extract_int(object, "x").value_or(0);
        spawn.y = json_extract_int(object, "y").value_or(0);
        spawns.push_back(spawn);

        pos = *object_end + 1;
    }

    return spawns;
}

std::vector<MapEntity> parse_entities(std::string_view json) {
    std::vector<MapEntity> entities;
    const auto marker = "\"entities\":";
    const auto start = json.find(marker);
    if (start == std::string_view::npos) {
        return entities;
    }

    const auto array_start = json.find('[', start);
    if (array_start == std::string_view::npos) {
        return entities;
    }

    const auto array_end = find_matching_bracket(json, array_start);
    if (!array_end) {
        return entities;
    }

    std::size_t pos = array_start;
    while (true) {
        const auto object_start = json.find('{', pos);
        if (object_start == std::string_view::npos || object_start > *array_end) {
            break;
        }
        const auto object_end = find_matching_brace(json, object_start);
        if (!object_end || *object_end > *array_end) {
            break;
        }

        const auto object = json.substr(object_start, *object_end - object_start + 1);
        MapEntity entity;
        entity.type = json_extract_string(object, "type").value_or("");
        entity.x = json_extract_int(object, "x").value_or(0);
        entity.y = json_extract_int(object, "y").value_or(0);
        entity.width = static_cast<float>(json_extract_int(object, "width").value_or(0));
        entity.height = static_cast<float>(json_extract_int(object, "height").value_or(0));

        if (entity.width <= 0.0f || entity.height <= 0.0f) {
            const auto dims = entity_dimensions(entity.type);
            if (dims) {
                entity.width = dims->width;
                entity.height = dims->height;
            }
        }

        entities.push_back(entity);
        pos = *object_end + 1;
    }

    return entities;
}

void parse_world_size(std::string_view json, TileMap& map) {
    const auto marker = "\"world\":";
    const auto start = json.find(marker);
    if (start != std::string_view::npos) {
        const auto brace = json.find('{', start);
        if (brace != std::string_view::npos) {
            const auto end = find_matching_brace(json, brace);
            if (end) {
                const auto block = json.substr(brace, *end - brace + 1);
                map.width = json_extract_int(block, "width").value_or(map.width);
                map.height = json_extract_int(block, "height").value_or(map.height);
                return;
            }
        }
    }

    map.width = json_extract_int(json, "width").value_or(map.width);
    map.height = json_extract_int(json, "height").value_or(map.height);
}

TileMap parse_tile_map_json(std::string_view json) {
    const auto format = json_extract_string(json, "format");
    if (!format || *format != TileMap::kFormatId) {
        throw std::runtime_error("Unsupported tile map format");
    }

    TileMap map;
    map.version = json_extract_int(json, "version").value_or(1);
    map.id = json_extract_string(json, "id").value_or("");
    map.name = json_extract_string(json, "name").value_or("");
    map.author = json_extract_string(json, "author").value_or("");
    map.description = json_extract_string(json, "description").value_or("");
    map.sky_asset = json_extract_string(json, "sky").value_or("");
    map.music_asset = json_extract_string(json, "music").value_or("");
    parse_world_size(json, map);
    map.tileset = parse_tileset(json);
    map.layers = parse_layers(json);
    map.spawns = parse_spawns(json);
    map.entities = parse_entities(json);
    return map;
}

} // namespace

TileMap load_tile_map_json(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Failed to open tile map JSON: " + path.string());
    }

    std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (json.empty()) {
        throw std::runtime_error("Empty tile map JSON: " + path.string());
    }
    return load_tile_map_json_string(json);
}

TileMap load_tile_map_json_string(std::string_view json) {
    return parse_tile_map_json(json);
}

} // namespace d2df::engine::map
