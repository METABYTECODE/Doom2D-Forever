#include <d2df/resources/content_catalog.hpp>

#include <fstream>
#include <stdexcept>

#include <d2df/resources/legacy_ref.hpp>
#include <d2df/tools/text_encoding.hpp>

namespace d2df::resources {
namespace {

std::optional<std::string> json_extract_string(std::string_view object, std::string_view key) {
    const std::string needle = std::string("\"") + std::string(key) + "\":\"";
    const auto start = object.find(needle);
    if (start == std::string_view::npos) {
        return std::nullopt;
    }

    std::string out;
    bool escape = false;
    for (std::size_t i = start + needle.size(); i < object.size(); ++i) {
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

std::optional<std::string> json_extract_legacy_ref(std::string_view line) {
    const std::string marker = "\"legacy_refs\":[\"";
    const auto start = line.find(marker);
    if (start == std::string_view::npos) {
        return std::nullopt;
    }

    std::string out;
    bool escape = false;
    for (std::size_t i = start + marker.size(); i < line.size(); ++i) {
        const char c = line[i];
        if (escape) {
            out.push_back(c);
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

std::optional<std::string> json_extract_quoted(std::string_view text, std::size_t quote_pos) {
    if (quote_pos >= text.size() || text[quote_pos] != '"') {
        return std::nullopt;
    }

    std::string out;
    bool escape = false;
    for (std::size_t i = quote_pos + 1; i < text.size(); ++i) {
        const char c = text[i];
        if (escape) {
            out.push_back(c);
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

std::optional<ContentAssetInfo> parse_asset_line(std::string_view line) {
    if (line.find("\"id\"") == std::string_view::npos) {
        return std::nullopt;
    }

    ContentAssetInfo asset;
    asset.id = json_extract_string(line, "id").value_or("");
    asset.type = json_extract_string(line, "type").value_or("");
    asset.path = json_extract_string(line, "path").value_or("");
    if (asset.id.empty() || asset.path.empty()) {
        return std::nullopt;
    }

    if (const auto legacy = json_extract_legacy_ref(line)) {
        asset.legacy_refs.push_back(*legacy);
    }
    return asset;
}

} // namespace

void ContentCatalog::load(const std::filesystem::path& content_root) {
    content_root_ = content_root;
    assets_.clear();
    assets_by_id_.clear();
    aliases_.clear();

    const auto manifest_path = content_root / "manifest.json";
    std::ifstream in(manifest_path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Failed to open content manifest: " + manifest_path.string());
    }

    std::string line;
    while (std::getline(in, line)) {
        if (auto asset = parse_asset_line(line)) {
            assets_by_id_[asset->id] = assets_.size();
            assets_.push_back(std::move(*asset));
        }
    }

    load_aliases(content_root / "aliases.json");
}

void ContentCatalog::load_aliases(const std::filesystem::path& aliases_path) {
    std::ifstream in(aliases_path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Failed to open aliases: " + aliases_path.string());
    }

    std::string line;
    while (std::getline(in, line)) {
        const auto key_start = line.find('"');
        if (key_start == std::string::npos) {
            continue;
        }

        const auto colon = line.find("\": \"", key_start);
        if (colon == std::string::npos) {
            continue;
        }

        const std::string key = json_extract_quoted(line, key_start).value_or("");
        const std::string value = json_extract_quoted(line, colon + 3).value_or("");
        if (key.empty() || value.empty()) {
            continue;
        }

        aliases_[normalize_legacy_ref(key)] = value;
        const auto key_utf8 = d2df::tools::cp1251_to_utf8(key);
        if (key_utf8 != key) {
            aliases_[normalize_legacy_ref(key_utf8)] = value;
        }
        if (const auto mojibake = d2df::tools::utf8_latin1_mojibake_to_utf8(key)) {
            aliases_[normalize_legacy_ref(*mojibake)] = value;
        }
    }
}

const ContentAssetInfo* ContentCatalog::find_asset(std::string_view id) const {
    const auto it = assets_by_id_.find(std::string(id));
    if (it == assets_by_id_.end()) {
        return nullptr;
    }
    return &assets_[it->second];
}

std::optional<std::string> ContentCatalog::resolve_alias(std::string_view legacy_ref) const {
    if (legacy_ref.empty()) {
        return std::nullopt;
    }

    const auto normalized = normalize_legacy_ref(legacy_ref);
    if (const auto it = aliases_.find(normalized); it != aliases_.end()) {
        return it->second;
    }

    const auto decoded = normalize_legacy_ref(d2df::tools::cp1251_to_utf8(legacy_ref));
    if (const auto it = aliases_.find(decoded); it != aliases_.end()) {
        return it->second;
    }

    if (const auto mojibake = d2df::tools::utf8_latin1_mojibake_to_utf8(legacy_ref)) {
        const auto mojibake_norm = normalize_legacy_ref(*mojibake);
        if (const auto it = aliases_.find(mojibake_norm); it != aliases_.end()) {
            return it->second;
        }
    }

    return std::nullopt;
}

std::filesystem::path ContentCatalog::asset_path(std::string_view id) const {
    const auto* asset = find_asset(id);
    if (asset == nullptr) {
        return {};
    }
    return content_root_ / asset->path;
}

} // namespace d2df::resources
