#include <d2df/tools/legacy_map.hpp>

#include <array>
#include <cstring>
#include <stdexcept>

#include <d2df/tools/text_encoding.hpp>

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

std::uint32_t read_u32(std::span<const std::uint8_t> data, std::size_t offset) {
    std::uint32_t value = 0;
    std::memcpy(&value, data.data() + offset, sizeof(value));
    return value;
}

MapPoint read_point(std::span<const std::uint8_t> data, std::size_t offset) {
    return {read_i32(data, offset), read_i32(data, offset + 4)};
}

MapSize read_size_wh(std::span<const std::uint8_t> data, std::size_t offset) {
    return {read_u16(data, offset), read_u16(data, offset + 2)};
}

LegacyMapHeader parse_header(std::span<const std::uint8_t> data) {
    if (data.size() < 452) {
        throw std::runtime_error("Map header block too small");
    }

    LegacyMapHeader header;
    header.name = read_legacy_string(data, 0, 32);
    header.author = read_legacy_string(data, 32, 32);
    header.description = read_legacy_string(data, 64, 256);
    header.music = read_legacy_string(data, 320, 64);
    header.sky = read_legacy_string(data, 384, 64);
    header.size = read_size_wh(data, 448);
    return header;
}

LegacyTexture parse_texture(std::span<const std::uint8_t> data) {
    if (data.size() < 65) {
        throw std::runtime_error("Texture record too small");
    }
    LegacyTexture texture;
    texture.path = read_legacy_string(data, 0, 64);
    texture.animated = data[64] != 0;
    return texture;
}

LegacyPanel parse_panel(std::span<const std::uint8_t> data) {
    if (data.size() < 18) {
        throw std::runtime_error("Panel record too small");
    }
    LegacyPanel panel;
    panel.position = read_point(data, 0);
    panel.size = read_size_wh(data, 8);
    panel.texture = read_u16(data, 12);
    panel.type = read_u16(data, 14);
    panel.alpha = data[16];
    panel.flags = data[17];
    return panel;
}

LegacyItem parse_item(std::span<const std::uint8_t> data) {
    if (data.size() < 10) {
        throw std::runtime_error("Item record too small");
    }
    LegacyItem item;
    item.position = read_point(data, 0);
    item.type = data[8];
    item.options = data[9];
    return item;
}

LegacyArea parse_area(std::span<const std::uint8_t> data) {
    if (data.size() < 10) {
        throw std::runtime_error("Area record too small");
    }
    LegacyArea area;
    area.position = read_point(data, 0);
    area.type = data[8];
    area.direction = data[9];
    return area;
}

LegacyMonster parse_monster(std::span<const std::uint8_t> data) {
    if (data.size() < 10) {
        throw std::runtime_error("Monster record too small");
    }
    LegacyMonster monster;
    monster.position = read_point(data, 0);
    monster.type = data[8];
    monster.direction = data[9];
    return monster;
}

LegacyTrigger parse_trigger(std::span<const std::uint8_t> data) {
    if (data.size() < 148) {
        throw std::runtime_error("Trigger record too small");
    }
    LegacyTrigger trigger;
    trigger.position = read_point(data, 0);
    trigger.size = read_size_wh(data, 8);
    trigger.enabled = data[12] != 0;
    trigger.texture_panel = read_i32(data, 13);
    trigger.type = data[17];
    trigger.activate_type = data[18];
    trigger.keys = data[19];
    std::memcpy(trigger.triggerdata.data(), data.data() + 20, 128);
    return trigger;
}

template <typename ParseFn>
void parse_records(std::span<const std::uint8_t> payload, std::size_t record_size, ParseFn parse,
                   auto& out) {
    if (payload.empty()) {
        return;
    }
    if (payload.size() % record_size != 0) {
        throw std::runtime_error("Invalid record block size");
    }
    const std::size_t count = payload.size() / record_size;
    out.reserve(out.size() + count);
    for (std::size_t i = 0; i < count; ++i) {
        out.push_back(parse(payload.subspan(i * record_size, record_size)));
    }
}

} // namespace

std::string read_legacy_string(std::span<const std::uint8_t> data, std::size_t offset,
                               std::size_t max_len) {
    std::string raw;
    raw.reserve(max_len);
    for (std::size_t i = 0; i < max_len && offset + i < data.size(); ++i) {
        const char c = static_cast<char>(data[offset + i]);
        if (c == '\0') {
            break;
        }
        raw.push_back(c);
    }
    return decode_legacy_text(raw);
}

LegacyMapDocument parse_legacy_mapbin(std::span<const std::uint8_t> data) {
    if (data.size() < 4 || data[0] != 'M' || data[1] != 'A' || data[2] != 'P' || data[3] != 0x01) {
        throw std::runtime_error("Invalid map signature (expected MAP\\x01)");
    }

    LegacyMapDocument document;
    std::size_t pos = 4;
    while (pos < data.size()) {
        const std::uint8_t block_type = data[pos++];
        if (block_type == 0) {
            break;
        }
        if (pos + 8 > data.size()) {
            throw std::runtime_error("Truncated map block header");
        }

        const std::uint32_t reserved = read_u32(data, pos);
        (void)reserved;
        pos += 4;
        const std::uint32_t size = read_u32(data, pos);
        pos += 4;
        if (pos + size > data.size()) {
            throw std::runtime_error("Truncated map block payload");
        }

        const auto payload = data.subspan(pos, size);
        pos += size;

        switch (block_type) {
        case 7:
            document.header = parse_header(payload);
            break;
        case 1:
            parse_records(payload, 65, parse_texture, document.textures);
            break;
        case 2:
            parse_records(payload, 18, parse_panel, document.panels);
            break;
        case 3:
            parse_records(payload, 10, parse_item, document.items);
            break;
        case 4:
            parse_records(payload, 10, parse_area, document.areas);
            break;
        case 5:
            parse_records(payload, 10, parse_monster, document.monsters);
            break;
        case 6:
            parse_records(payload, 148, parse_trigger, document.triggers);
            break;
        default:
            throw std::runtime_error("Unknown map block type: " + std::to_string(block_type));
        }
    }

    return document;
}

} // namespace d2df::tools
