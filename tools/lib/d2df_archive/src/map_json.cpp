#include <d2df/tools/map_json.hpp>

#include <cstring>
#include <span>
#include <sstream>

#include <d2df/tools/json_util.hpp>
#include <d2df/tools/map_enum_names.hpp>
#include <d2df/tools/text_encoding.hpp>
#include <d2df/resources/legacy_ref.hpp>

namespace d2df::tools {
namespace {

std::int32_t read_i32(std::span<const std::uint8_t> data, std::size_t offset) {
    std::int32_t value = 0;
    std::memcpy(&value, data.data() + offset, sizeof(value));
    return value;
}

std::uint16_t read_u16(std::span<const std::uint8_t> data, std::size_t offset) {
    std::uint16_t value = 0;
    std::memcpy(&value, data.data() + offset, sizeof(value));
    return value;
}

std::string rewrite_resource_ref(const d2df::resources::ContentCatalog& catalog,
                                 std::string_view field, std::string_view legacy_ref,
                                 std::string_view map_legacy_ref,
                                 std::vector<MapRefIssue>& issues) {
    if (legacy_ref.empty()) {
        return "";
    }

    if (d2df::resources::is_builtin_texture_ref(legacy_ref)) {
        return std::string(legacy_ref);
    }

    const auto qualified =
        d2df::resources::qualify_map_legacy_ref(legacy_ref, map_legacy_ref);
    if (const auto asset_id = catalog.resolve_alias(qualified)) {
        return *asset_id;
    }

    issues.push_back({std::string(field), std::string(legacy_ref), "no alias in content catalog"});
    return std::string(legacy_ref);
}

void write_string_array(std::ostringstream& out, const std::vector<std::string>& values) {
    out << '[';
    for (std::size_t i = 0; i < values.size(); ++i) {
        out << '"' << json_escape(values[i]) << '"';
        if (i + 1 < values.size()) {
            out << ',';
        }
    }
    out << ']';
}

void write_point(std::ostringstream& out, const MapPoint& point) {
    out << "{\"x\":" << point.x << ",\"y\":" << point.y << '}';
}

void write_size(std::ostringstream& out, const MapSize& size) {
    out << "{\"width\":" << size.width << ",\"height\":" << size.height << '}';
}

std::string read_trigger_string(std::span<const std::uint8_t> data, std::size_t offset,
                                std::size_t max_len) {
    std::string raw;
    for (std::size_t i = 0; i < max_len && offset + i < data.size(); ++i) {
        const char c = static_cast<char>(data[offset + i]);
        if (c == '\0') {
            break;
        }
        raw.push_back(c);
    }
    return decode_legacy_text(raw);
}

void write_trigger_data(std::ostringstream& out, const LegacyTrigger& trigger,
                        const d2df::resources::ContentCatalog& catalog,
                        std::vector<MapRefIssue>& issues, std::string_view field_prefix,
                        std::string_view map_legacy_ref) {
    const auto& data = trigger.triggerdata;
    out << "{";
    bool first = true;
    auto comma = [&]() {
        if (!first) {
            out << ',';
        }
        first = false;
    };

    switch (trigger.type) {
    case 1: { // TRIGGER_EXIT
        const auto map_name = read_trigger_string(data, 0, 16);
        comma();
        out << "\"map\":\"" << json_escape(map_name) << '"';
        break;
    }
    case 2: { // TRIGGER_TELEPORT
        comma();
        out << "\"target\":";
        write_point(out, {read_i32(data, 0), read_i32(data, 4)});
        comma();
        out << "\"d2d\":" << (data[8] != 0 ? "true" : "false");
        comma();
        out << "\"silent\":" << (data[9] != 0 ? "true" : "false");
        comma();
        out << "\"direction\":\"" << json_escape(dir_type_name(data[10])) << '"';
        break;
    }
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 11:
    case 12:
    case 13: {
        std::int32_t panel_id = 0;
        std::memcpy(&panel_id, data.data(), sizeof(panel_id));
        comma();
        out << "\"panelid\":" << panel_id;
        comma();
        out << "\"silent\":" << (data[4] != 0 ? "true" : "false");
        comma();
        out << "\"d2d\":" << (data[5] != 0 ? "true" : "false");
        break;
    }
    case 9:  // TRIGGER_PRESS
    case 15: // TRIGGER_ON
    case 16: // TRIGGER_OFF
    case 17: { // TRIGGER_ONOFF
        comma();
        out << "\"position\":";
        write_point(out, {read_i32(data, 0), read_i32(data, 4)});
        comma();
        out << "\"size\":";
        write_size(out, {read_u16(data, 8), read_u16(data, 10)});
        comma();
        out << "\"wait\":" << read_u16(data, 12);
        comma();
        out << "\"count\":" << read_u16(data, 14);
        comma();
        out << "\"ext_random\":" << (data[20] != 0 ? "true" : "false");
        break;
    }
    case 18: { // TRIGGER_SOUND
        const auto sound = read_trigger_string(data, 0, 64);
        comma();
        out << "\"sound\":\"" << json_escape(rewrite_resource_ref(
                                   catalog, field_prefix, sound, map_legacy_ref, issues))
            << '"';
        comma();
        out << "\"volume\":" << static_cast<int>(data[64]);
        comma();
        out << "\"pan\":" << static_cast<int>(data[65]);
        comma();
        out << "\"local\":" << (data[66] != 0 ? "true" : "false");
        comma();
        out << "\"play_count\":" << static_cast<int>(data[67]);
        comma();
        out << "\"sound_switch\":" << (data[68] != 0 ? "true" : "false");
        break;
    }
    case 21: { // TRIGGER_MUSIC
        const auto music = read_trigger_string(data, 0, 64);
        comma();
        out << "\"music\":\"" << json_escape(rewrite_resource_ref(
                                    catalog, field_prefix, music, map_legacy_ref, issues))
            << '"';
        comma();
        out << "\"action\":" << static_cast<int>(data[64]);
        break;
    }
    case 24: { // TRIGGER_MESSAGE
        const auto text = read_trigger_string(data, 2, 100);
        comma();
        out << "\"kind\":" << static_cast<int>(data[0]);
        comma();
        out << "\"dest\":" << static_cast<int>(data[1]);
        comma();
        out << "\"text\":\"" << json_escape(text) << '"';
        break;
    }
    default:
        break;
    }

    if (first) {
        out << '}';
    } else {
        out << '}';
    }
}

} // namespace

std::string write_map_json_v1(const LegacyMapDocument& document,
                              const d2df::resources::ContentCatalog& catalog,
                              const MapJsonOptions& options, std::vector<MapRefIssue>& issues) {
    std::ostringstream out;
    out << "{\n";
    out << "  \"version\": 1,\n";
    out << "  \"id\": \"" << json_escape(options.asset_id) << "\",\n";
    out << "  \"name\": \"" << json_escape(document.header.name) << "\",\n";
    out << "  \"author\": \"" << json_escape(document.header.author) << "\",\n";
    out << "  \"description\": \"" << json_escape(document.header.description) << "\",\n";
    out << "  \"size\": ";
    write_size(out, document.header.size);
    out << ",\n";
    out << "  \"music\": \"" << json_escape(rewrite_resource_ref(
                                   catalog, "music", document.header.music, options.legacy_ref,
                                   issues))
        << "\",\n";
    out << "  \"sky\": \"" << json_escape(rewrite_resource_ref(
                                 catalog, "sky", document.header.sky, options.legacy_ref, issues))
        << "\",\n";

    out << "  \"textures\": [\n";
    for (std::size_t i = 0; i < document.textures.size(); ++i) {
        const auto& texture = document.textures[i];
        out << "    {\"path\":\""
            << json_escape(rewrite_resource_ref(catalog, "textures[" + std::to_string(i) + "].path",
                                                texture.path, options.legacy_ref, issues))
            << "\",\"animated\":" << (texture.animated ? "true" : "false") << '}';
        if (i + 1 < document.textures.size()) {
            out << ',';
        }
        out << '\n';
    }
    out << "  ],\n";

    out << "  \"panels\": [\n";
    for (std::size_t i = 0; i < document.panels.size(); ++i) {
        const auto& panel = document.panels[i];
        out << "    {\"position\":";
        write_point(out, panel.position);
        out << ",\"size\":";
        write_size(out, panel.size);
        out << ",\"texture\":" << panel.texture << ",\"type\":";
        write_string_array(out, panel_type_names(panel.type));
        out << ",\"alpha\":" << static_cast<int>(panel.alpha) << ",\"flags\":";
        write_string_array(out, panel_flag_names(panel.flags));
        out << '}';
        if (i + 1 < document.panels.size()) {
            out << ',';
        }
        out << '\n';
    }
    out << "  ],\n";

    out << "  \"items\": [\n";
    for (std::size_t i = 0; i < document.items.size(); ++i) {
        const auto& item = document.items[i];
        out << "    {\"position\":";
        write_point(out, item.position);
        out << ",\"type\":\"" << json_escape(item_type_name(item.type)) << "\",\"options\":";
        write_string_array(out, item_option_names(item.options));
        out << '}';
        if (i + 1 < document.items.size()) {
            out << ',';
        }
        out << '\n';
    }
    out << "  ],\n";

    out << "  \"monsters\": [\n";
    for (std::size_t i = 0; i < document.monsters.size(); ++i) {
        const auto& monster = document.monsters[i];
        out << "    {\"position\":";
        write_point(out, monster.position);
        out << ",\"type\":\"" << json_escape(monster_type_name(monster.type))
            << "\",\"direction\":\"" << json_escape(dir_type_name(monster.direction)) << "\"}";
        if (i + 1 < document.monsters.size()) {
            out << ',';
        }
        out << '\n';
    }
    out << "  ],\n";

    out << "  \"areas\": [\n";
    for (std::size_t i = 0; i < document.areas.size(); ++i) {
        const auto& area = document.areas[i];
        out << "    {\"position\":";
        write_point(out, area.position);
        out << ",\"type\":\"" << json_escape(area_type_name(area.type))
            << "\",\"direction\":\"" << json_escape(dir_type_name(area.direction)) << "\"}";
        if (i + 1 < document.areas.size()) {
            out << ',';
        }
        out << '\n';
    }
    out << "  ],\n";

    out << "  \"triggers\": [\n";
    for (std::size_t i = 0; i < document.triggers.size(); ++i) {
        const auto& trigger = document.triggers[i];
        const std::string prefix = "triggers[" + std::to_string(i) + "]";
        out << "    {\"position\":";
        write_point(out, trigger.position);
        out << ",\"size\":";
        write_size(out, trigger.size);
        out << ",\"enabled\":" << (trigger.enabled ? "true" : "false");
        out << ",\"texture_panel\":" << trigger.texture_panel;
        out << ",\"type\":\"" << json_escape(trigger_type_name(trigger.type)) << "\"";
        out << ",\"activate_type\":";
        write_string_array(out, activate_type_names(trigger.activate_type));
        out << ",\"keys\":";
        write_string_array(out, key_names(trigger.keys));
        out << ",\"data\":";
        write_trigger_data(out, trigger, catalog, issues, prefix, options.legacy_ref);
        out << '}';
        if (i + 1 < document.triggers.size()) {
            out << ',';
        }
        out << '\n';
    }
    out << "  ],\n";

    out << "  \"meta\": {\n";
    out << "    \"legacy_ref\": \"" << json_escape(options.legacy_ref) << "\",\n";
    out << "    \"source_path\": \"" << json_escape(options.source_path) << "\",\n";
    out << "    \"source_format\": \"mapbin\",\n";
    out << "    \"unresolved_refs\": " << issues.size() << "\n";
    out << "  }\n";
    out << "}\n";
    return out.str();
}

} // namespace d2df::tools
