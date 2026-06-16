#include <d2df/map/map_json_loader.hpp>

#include <fstream>
#include <stdexcept>

#include <d2df/map/area_types.hpp>
#include <d2df/map/panel_types.hpp>
#include <d2df/map/trigger_types.hpp>

namespace d2df::map {
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

std::uint16_t parse_type_bitset(std::string_view object) {
    const auto marker = "\"type\":";
    const auto start = object.find(marker);
    if (start == std::string_view::npos) {
        return 0;
    }

    const auto array_start = object.find('[', start);
    if (array_start == std::string_view::npos) {
        return 0;
    }

    const auto array_end = find_matching_bracket(object, array_start);
    if (!array_end) {
        return 0;
    }

    std::uint16_t bits = 0;
    const auto array = object.substr(array_start, *array_end - array_start + 1);
    std::size_t pos = 0;
    while ((pos = array.find('"', pos)) != std::string_view::npos) {
        const auto name_start = pos + 1;
        const auto name_end = array.find('"', name_start);
        if (name_end == std::string_view::npos) {
            break;
        }
        bits |= panel_type_from_name(array.substr(name_start, name_end - name_start));
        pos = name_end + 1;
    }
    return bits;
}

std::uint8_t parse_flag_bitset(std::string_view object) {
    const auto marker = "\"flags\":";
    const auto start = object.find(marker);
    if (start == std::string_view::npos) {
        return 0;
    }

    const auto array_start = object.find('[', start);
    if (array_start == std::string_view::npos) {
        return 0;
    }

    const auto array_end = find_matching_bracket(object, array_start);
    if (!array_end) {
        return 0;
    }

    std::uint8_t bits = 0;
    const auto array = object.substr(array_start, *array_end - array_start + 1);
    if (array.find("PANEL_FLAG_HIDE") != std::string_view::npos) {
        bits |= PANEL_FLAG_HIDE;
    }
    if (array.find("PANEL_FLAG_BLENDING") != std::string_view::npos) {
        bits |= PANEL_FLAG_BLENDING;
    }
    return bits;
}

MapPoint parse_point(std::string_view object, std::string_view key) {
    const std::string marker = std::string("\"") + std::string(key) + "\":";
    const auto start = object.find(marker);
    if (start == std::string_view::npos) {
        return {};
    }

    const auto brace = object.find('{', start);
    if (brace == std::string_view::npos) {
        return {};
    }

    const auto end = find_matching_brace(object, brace);
    if (!end) {
        return {};
    }

    const auto block = object.substr(brace, *end - brace + 1);
    MapPoint point;
    point.x = json_extract_int(block, "x").value_or(0);
    point.y = json_extract_int(block, "y").value_or(0);
    return point;
}

MapSize parse_size(std::string_view object, std::string_view key) {
    const std::string marker = std::string("\"") + std::string(key) + "\":";
    const auto start = object.find(marker);
    if (start == std::string_view::npos) {
        return {};
    }

    const auto brace = object.find('{', start);
    if (brace == std::string_view::npos) {
        return {};
    }

    const auto end = find_matching_brace(object, brace);
    if (!end) {
        return {};
    }

    const auto block = object.substr(brace, *end - brace + 1);
    MapSize size;
    size.width = json_extract_int(block, "width").value_or(0);
    size.height = json_extract_int(block, "height").value_or(0);
    return size;
}

std::vector<MapTexture> parse_textures(std::string_view json) {
    std::vector<MapTexture> textures;
    const auto marker = "\"textures\":";
    const auto start = json.find(marker);
    if (start == std::string_view::npos) {
        return textures;
    }

    const auto array_start = json.find('[', start);
    if (array_start == std::string_view::npos) {
        return textures;
    }

    const auto array_end = find_matching_bracket(json, array_start);
    if (!array_end) {
        return textures;
    }

    const auto array = json.substr(array_start, *array_end - array_start + 1);
    for (std::size_t i = 0; i < array.size();) {
        const auto obj_start = array.find('{', i);
        if (obj_start == std::string_view::npos) {
            break;
        }

        const auto obj_end = find_matching_brace(array, obj_start);
        if (!obj_end) {
            break;
        }

        const auto object = array.substr(obj_start, *obj_end - obj_start + 1);
        MapTexture texture;
        texture.path = json_extract_string(object, "path").value_or("");
        texture.animated = object.find("\"animated\":true") != std::string_view::npos;
        textures.push_back(std::move(texture));
        i = *obj_end + 1;
    }
    return textures;
}

std::vector<MapPanel> parse_panels(std::string_view json) {
    std::vector<MapPanel> panels;
    const auto marker = "\"panels\":";
    const auto start = json.find(marker);
    if (start == std::string_view::npos) {
        return panels;
    }

    const auto array_start = json.find('[', start);
    if (array_start == std::string_view::npos) {
        return panels;
    }

    const auto array_end = find_matching_bracket(json, array_start);
    if (!array_end) {
        return panels;
    }

    const auto array = json.substr(array_start, *array_end - array_start + 1);
    for (std::size_t i = 0; i < array.size();) {
        const auto obj_start = array.find('{', i);
        if (obj_start == std::string_view::npos) {
            break;
        }

        const auto obj_end = find_matching_brace(array, obj_start);
        if (!obj_end) {
            break;
        }

        const auto object = array.substr(obj_start, *obj_end - obj_start + 1);
        MapPanel panel;
        panel.position = parse_point(object, "position");
        panel.size = parse_size(object, "size");
        panel.texture = static_cast<std::uint16_t>(json_extract_int(object, "texture").value_or(0));
        panel.type = parse_type_bitset(object);
        panel.alpha = static_cast<std::uint8_t>(json_extract_int(object, "alpha").value_or(0));
        panel.flags = parse_flag_bitset(object);
        panels.push_back(std::move(panel));
        i = *obj_end + 1;
    }
    return panels;
}

std::vector<MapArea> parse_areas(std::string_view json) {
    std::vector<MapArea> areas;
    const auto marker = "\"areas\":";
    const auto start = json.find(marker);
    if (start == std::string_view::npos) {
        return areas;
    }

    const auto array_start = json.find('[', start);
    if (array_start == std::string_view::npos) {
        return areas;
    }

    const auto array_end = find_matching_bracket(json, array_start);
    if (!array_end) {
        return areas;
    }

    const auto array = json.substr(array_start, *array_end - array_start + 1);
    for (std::size_t i = 0; i < array.size();) {
        const auto obj_start = array.find('{', i);
        if (obj_start == std::string_view::npos) {
            break;
        }

        const auto obj_end = find_matching_brace(array, obj_start);
        if (!obj_end) {
            break;
        }

        const auto object = array.substr(obj_start, *obj_end - obj_start + 1);
        MapArea area;
        area.position = parse_point(object, "position");
        area.type = area_type_from_name(json_extract_string(object, "type").value_or(""));
        areas.push_back(std::move(area));
        i = *obj_end + 1;
    }
    return areas;
}

std::optional<std::string_view> json_data_block(std::string_view object) {
    const auto data_marker = "\"data\":";
    const auto data_start = object.find(data_marker);
    if (data_start == std::string_view::npos) {
        return std::nullopt;
    }

    const auto brace = object.find('{', data_start);
    if (brace == std::string_view::npos) {
        return std::nullopt;
    }

    const auto end = find_matching_brace(object, brace);
    if (!end) {
        return std::nullopt;
    }

    return object.substr(brace, *end - brace + 1);
}

std::optional<std::int32_t> json_extract_panelid(std::string_view object) {
    const auto data = json_data_block(object);
    if (!data) {
        return std::nullopt;
    }
    return json_extract_int(*data, "panelid");
}

std::vector<MapTrigger> parse_triggers(std::string_view json) {
    std::vector<MapTrigger> triggers;
    const auto marker = "\"triggers\":";
    const auto start = json.find(marker);
    if (start == std::string_view::npos) {
        return triggers;
    }

    const auto array_start = json.find('[', start);
    if (array_start == std::string_view::npos) {
        return triggers;
    }

    const auto array_end = find_matching_bracket(json, array_start);
    if (!array_end) {
        return triggers;
    }

    const auto array = json.substr(array_start, *array_end - array_start + 1);
    for (std::size_t i = 0; i < array.size();) {
        const auto obj_start = array.find('{', i);
        if (obj_start == std::string_view::npos) {
            break;
        }

        const auto obj_end = find_matching_brace(array, obj_start);
        if (!obj_end) {
            break;
        }

        const auto object = array.substr(obj_start, *obj_end - obj_start + 1);
        MapTrigger trigger;
        trigger.position = parse_point(object, "position");
        trigger.size = parse_size(object, "size");
        trigger.type = trigger_type_from_name(json_extract_string(object, "type").value_or(""));
        trigger.enabled = object.find("\"enabled\":false") == std::string_view::npos;
        trigger.target_panel = json_extract_panelid(object).value_or(-1);

        const auto activate_marker = "\"activate_type\":";
        const auto activate_start = object.find(activate_marker);
        if (activate_start != std::string_view::npos) {
            const auto activate_array_start = object.find('[', activate_start);
            if (activate_array_start != std::string_view::npos) {
                const auto activate_array_end = find_matching_bracket(object, activate_array_start);
                if (activate_array_end) {
                    const auto activate_snippet = object.substr(
                        activate_array_start, *activate_array_end - activate_array_start + 1);
                    trigger.activate = activate_type_from_json(activate_snippet);
                }
            }
        }

        if (const auto data = json_data_block(object)) {
            trigger.teleport_target = parse_point(*data, "target");
            trigger.press_position = parse_point(*data, "position");
            trigger.press_size = parse_size(*data, "size");
            trigger.d2d = json_extract_bool(*data, "d2d").value_or(false);
        }

        triggers.push_back(std::move(trigger));
        i = *obj_end + 1;
    }
    return triggers;
}

} // namespace

MapDocument load_map_json_v1(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Failed to open map JSON: " + path.string());
    }

    std::string json((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (json.empty()) {
        throw std::runtime_error("Empty map JSON: " + path.string());
    }

    MapDocument document;
    document.version = json_extract_int(json, "version").value_or(0);
    document.id = json_extract_string(json, "id").value_or("");
    document.name = json_extract_string(json, "name").value_or("");
    document.author = json_extract_string(json, "author").value_or("");
    document.description = json_extract_string(json, "description").value_or("");
    document.music = json_extract_string(json, "music").value_or("");
    document.sky = json_extract_string(json, "sky").value_or("");
    document.size = parse_size(json, "size");
    document.textures = parse_textures(json);
    document.panels = parse_panels(json);
    document.areas = parse_areas(json);
    document.triggers = parse_triggers(json);

    if (document.version != 1) {
        throw std::runtime_error("Unsupported map JSON version in " + path.string());
    }
    if (document.panels.empty()) {
        throw std::runtime_error("Map JSON has no panels: " + path.string());
    }

    return document;
}

} // namespace d2df::map
