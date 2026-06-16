#include <d2df/tools/organize_pipeline.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <set>
#include <unordered_map>

#include <spdlog/spdlog.h>

#include <d2df/tools/json_util.hpp>
#include <d2df/tools/staging_manifest.hpp>

namespace d2df::tools {
namespace {

std::string lowercase_ascii(std::string_view text) {
    std::string out(text);
    for (char& c : out) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return out;
}

std::string archive_stem(std::string_view archive_name) {
    const auto stem = std::filesystem::path(archive_name).stem().string();
    return lowercase_ascii(stem);
}

std::string make_utf8_legacy_ref(const ContentAsset& asset) {
    std::string ref = asset.archive;
    ref.push_back(':');
    if (!asset.virtual_path.empty()) {
        ref += asset.virtual_path;
    }
    ref += asset.original_name;
    return ref;
}

struct OrganizeRule {
    std::string archive;
    std::string path_prefix;
    std::string content_dir;
    std::string asset_prefix;
    std::string asset_type;
    int priority = 0;
};

struct OrganizeOverride {
    std::string archive;
    std::string virtual_path;
    std::string slug;
    std::string content_dir;
    std::string asset_id;
};

bool iequals(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        const auto ca = static_cast<unsigned char>(a[i]);
        const auto cb = static_cast<unsigned char>(b[i]);
        if (std::tolower(ca) != std::tolower(cb)) {
            return false;
        }
    }
    return true;
}

bool path_starts_with_ci(std::string_view path, std::string_view prefix) {
    if (prefix.empty()) {
        return true;
    }
    if (path.size() < prefix.size()) {
        return false;
    }
    return iequals(path.substr(0, prefix.size()), prefix);
}

std::vector<OrganizeRule> default_rules() {
    return {
        {"game.WAD", "SOUNDS/", "audio/sfx/world", "sfx.world", "sfx", 100},
        {"game.WAD", "MSOUNDS/", "audio/sfx/monsters", "sfx.monster", "sfx", 100},
        {"game.WAD", "CHATSND/", "audio/sfx/ui", "sfx.ui", "sfx", 100},
        {"game.WAD", "PSOUNDS/", "audio/sfx/player", "sfx.player", "sfx", 100},
        {"game.WAD", "MUSIC/", "audio/music", "music", "music", 100},
        {"game.WAD", "FONTS/", "fonts", "font", "font", 100},
        {"game.WAD", "TEXTURES/", "textures/ui", "tex.ui", "texture", 100},
        {"game.WAD", "MTEXTURES/", "textures/monsters", "tex.monster", "texture", 100},
        {"game.WAD", "WEAPONS/", "textures/weapons", "tex.weapon", "texture", 100},
        {"game.WAD", "PARTICLES/", "textures/effects", "tex.fx", "texture", 100},
        {"game.WAD", "BLOOD/", "textures/effects/blood", "tex.blood", "texture", 100},
        {"standart.WAD", "D2DMUS/", "audio/music", "music", "music", 100},
        {"standart.WAD", "D2DTEXTURES/", "textures/tiles", "tex.tile", "texture", 100},
        {"standart.WAD", "STDTEXTURES/", "textures/tiles", "tex.tile", "texture", 100},
        {"standart.WAD", "D2DSKY/", "textures/skies", "tex.sky", "texture", 100},
        {"editor.WAD", "TEXTURES/", "textures/editor", "tex.editor", "texture", 90},
        {"flexui.wad", "flexui/fonts/", "fonts/flexui", "font.flexui", "font", 90},
        {"flexui.wad", "flexui/", "ui/flexui", "ui.flexui", "ui", 80},
        {"*", "MTEXTURES/", "textures/monsters", "tex.monster", "texture", 50},
        {"*", "TEXTURES/", "textures/tiles", "tex.tile", "texture", 45},
        {"*", "MUS_DOOM/", "audio/music", "music", "music", 45},
        {"*", "MUS_DOOM2/", "audio/music", "music", "music", 45},
        {"*", "D2DMUS/", "audio/music", "music", "music", 45},
        {"*", "SOUNDS/", "audio/sfx/models", "sfx.model", "sfx", 40},
        {"*", "WEAPONS/", "textures/weapons", "tex.weapon", "texture", 40},
        {"*", "FONTS/", "fonts", "font", "font", 40},
    };
}

std::optional<OrganizeRule> match_rule(const std::vector<OrganizeRule>& rules,
                                       const StagingRecord& record) {
    const auto archive = lowercase_ascii(record.archive);
    const auto& path = record.virtual_path;

    const OrganizeRule* best = nullptr;
    for (const auto& rule : rules) {
        if (rule.archive != "*" && !iequals(rule.archive, archive)) {
            continue;
        }
        if (!path_starts_with_ci(path, rule.path_prefix)) {
            continue;
        }
        if (best == nullptr || rule.priority > best->priority) {
            best = &rule;
        }
    }
    return best ? std::optional<OrganizeRule>{*best} : std::nullopt;
}

std::string asset_type_from_kind(std::string_view kind) {
    if (kind == "audio") {
        return "sfx";
    }
    if (kind == "image") {
        return "texture";
    }
    if (kind == "font") {
        return "font";
    }
    if (kind == "map") {
        return "map";
    }
    return "binary";
}

std::string make_asset_id(const OrganizeRule* rule, const StagingRecord& record,
                          const std::string& content_dir) {
    if (rule != nullptr) {
        return rule->asset_prefix + "." + record.slug;
    }
    if (record.kind == "map") {
        return "map." + archive_stem(record.archive) + "." + record.slug;
    }
    return "raw." + archive_stem(record.archive) + "." + record.slug;
}

std::string resolve_content_dir(const OrganizeRule* rule, const StagingRecord& record) {
    if (rule != nullptr) {
        if (rule->archive == "*" &&
            (rule->path_prefix == "MTEXTURES/" || rule->path_prefix == "SOUNDS/" ||
             rule->path_prefix == "WEAPONS/" || rule->path_prefix == "TEXTURES/" ||
             rule->path_prefix == "MUS_DOOM/" || rule->path_prefix == "MUS_DOOM2/" ||
             rule->path_prefix == "D2DMUS/")) {
            return rule->content_dir + "/" + archive_stem(record.archive);
        }
        return rule->content_dir;
    }
    if (record.kind == "map") {
        return "maps/legacy/" + archive_stem(record.archive);
    }
    return "_unclassified/" + archive_stem(record.archive);
}

std::string resolve_asset_type(const OrganizeRule* rule, const StagingRecord& record) {
    if (rule != nullptr) {
        return rule->asset_type;
    }
    return asset_type_from_kind(record.kind);
}

std::string unique_asset_id(std::string base, const StagingRecord& record,
                             std::unordered_map<std::string, std::size_t>& used_ids,
                             OrganizeStats& stats) {
    auto it = used_ids.find(base);
    if (it == used_ids.end()) {
        used_ids.emplace(base, 1);
        return base;
    }

    const auto disambiguated = base + "." + archive_stem(record.archive);
    it = used_ids.find(disambiguated);
    if (it == used_ids.end()) {
        used_ids.emplace(disambiguated, 1);
        ++stats.id_collisions_resolved;
        return disambiguated;
    }

    std::size_t suffix = ++it->second;
    const auto final_id = disambiguated + "_" + std::to_string(suffix);
    used_ids[final_id] = 1;
    ++stats.id_collisions_resolved;
    return final_id;
}

std::vector<OrganizeOverride> load_overrides(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        return {};
    }

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        spdlog::warn("Failed to open overrides: {}", path.string());
        return {};
    }

    std::vector<OrganizeOverride> overrides;
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    std::size_t pos = 0;
    while ((pos = content.find("\"content_dir\"", pos)) != std::string::npos) {
        const auto block_start = content.rfind('{', pos);
        const auto block_end = content.find('}', pos);
        if (block_start == std::string::npos || block_end == std::string::npos) {
            break;
        }

        const auto block = std::string_view(content).substr(block_start, block_end - block_start + 1);
        OrganizeOverride override_entry;
        override_entry.archive = json_extract_string(block, "archive").value_or("");
        override_entry.virtual_path = json_extract_string(block, "virtual_path").value_or("");
        override_entry.slug = json_extract_string(block, "slug").value_or("");
        override_entry.content_dir = json_extract_string(block, "content_dir").value_or("");
        override_entry.asset_id = json_extract_string(block, "asset_id").value_or("");
        if (!override_entry.content_dir.empty() && !override_entry.asset_id.empty()) {
            overrides.push_back(std::move(override_entry));
        }
        pos = block_end + 1;
    }

    spdlog::info("Loaded {} organize override(s)", overrides.size());
    return overrides;
}

const OrganizeOverride* find_override(const std::vector<OrganizeOverride>& overrides,
                                      const StagingRecord& record) {
    for (const auto& override_entry : overrides) {
        if (!override_entry.archive.empty() && !iequals(override_entry.archive, record.archive)) {
            continue;
        }
        if (!override_entry.virtual_path.empty() &&
            !path_starts_with_ci(record.virtual_path, override_entry.virtual_path)) {
            continue;
        }
        if (!override_entry.slug.empty() && override_entry.slug != record.slug) {
            continue;
        }
        return &override_entry;
    }
    return nullptr;
}

void copy_asset_file(const std::filesystem::path& src, const std::filesystem::path& dst,
                     bool dry_run) {
    if (dry_run) {
        return;
    }
    std::filesystem::create_directories(dst.parent_path());
    std::filesystem::copy_file(src, dst, std::filesystem::copy_options::overwrite_existing);
}

void write_content_manifest(const OrganizeOptions& options, const std::vector<ContentAsset>& assets,
                            const OrganizeStats& stats) {
    if (options.dry_run) {
        return;
    }

    const auto manifest_path = options.output_root / "manifest.json";
    std::filesystem::create_directories(options.output_root);

    std::ofstream out(manifest_path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Failed to write content manifest: " + manifest_path.string());
    }

    out << "{\n";
    out << "  \"version\": 1,\n";
    out << "  \"generator\": \"d2df-organize\",\n";
    out << "  \"staging_root\": \"" << json_escape(options.staging_root.generic_string()) << "\",\n";
    out << "  \"output_root\": \"" << json_escape(options.output_root.generic_string()) << "\",\n";
    out << "  \"stats\": { \"input_entries\":" << stats.input_entries
        << ", \"unique_entries\":" << stats.unique_entries << ", \"organized\":" << stats.organized
        << ", \"unclassified\":" << stats.unclassified
        << ", \"duplicates_skipped\":" << stats.duplicates_skipped
        << ", \"id_collisions_resolved\":" << stats.id_collisions_resolved
        << ", \"errors\":" << stats.errors << " },\n";
    out << "  \"assets\": [\n";

    for (std::size_t i = 0; i < assets.size(); ++i) {
        const auto& asset = assets[i];
        out << "    {";
        out << "\"id\":\"" << json_escape(asset.id) << "\",";
        out << "\"type\":\"" << json_escape(asset.type) << "\",";
        out << "\"path\":\"" << json_escape(asset.path) << "\",";
        out << "\"legacy_refs\":[\"" << json_escape(make_utf8_legacy_ref(asset)) << "\"],";
        out << "\"meta\":{";
        out << "\"original_name\":\"" << json_escape(asset.original_name) << "\",";
        out << "\"slug\":\"" << json_escape(asset.slug) << "\",";
        out << "\"archive\":\"" << json_escape(asset.archive) << "\",";
        out << "\"virtual_path\":\"" << json_escape(asset.virtual_path) << "\",";
        out << "\"format\":\"" << json_escape(asset.output_format) << "\",";
        out << "\"size_bytes\":" << asset.size_bytes;
        out << "}";
        out << "}";
        if (i + 1 < assets.size()) {
            out << ',';
        }
        out << '\n';
    }

    out << "  ]\n}\n";
    spdlog::info("Wrote {} ({} assets)", manifest_path.string(), assets.size());
}

void write_aliases(const OrganizeOptions& options, const std::vector<ContentAsset>& assets) {
    if (options.dry_run) {
        return;
    }

    const auto aliases_path = options.output_root / "aliases.json";
    std::ofstream out(aliases_path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Failed to write aliases: " + aliases_path.string());
    }

    out << "{\n  \"version\": 1,\n  \"aliases\": {\n";
    for (std::size_t i = 0; i < assets.size(); ++i) {
        const auto& asset = assets[i];
        out << "    \"" << json_escape(make_utf8_legacy_ref(asset)) << "\": \""
            << json_escape(asset.id) << "\"";
        if (i + 1 < assets.size()) {
            out << ',';
        }
        out << '\n';
    }
    out << "  }\n}\n";
    spdlog::info("Wrote {} ({} aliases)", aliases_path.string(), assets.size());
}

} // namespace

OrganizeStats run_organize(const OrganizeOptions& options, std::vector<ContentAsset>& assets) {
    OrganizeStats stats;
    const auto manifest_path = options.staging_root / "manifest.json";
    const auto all_records = load_staging_manifest(manifest_path);
    stats.input_entries = all_records.size();

    const auto records = dedupe_staging_records(all_records);
    stats.unique_entries = records.size();
    stats.duplicates_skipped = stats.input_entries - stats.unique_entries;

    auto overrides_path = options.overrides_path;
    if (!std::filesystem::exists(overrides_path)) {
        const auto fallback = options.staging_root.parent_path() / "organize_overrides.json";
        if (std::filesystem::exists(fallback)) {
            overrides_path = fallback;
        }
    }

    const auto rules = default_rules();
    const auto overrides = load_overrides(overrides_path);
    std::unordered_map<std::string, std::size_t> used_ids;

    for (const auto& record : records) {
        const auto src = options.staging_root / record.disk_path;
        if (!std::filesystem::exists(src)) {
            spdlog::error("Missing staging file: {}", src.string());
            ++stats.errors;
            continue;
        }

        const OrganizeOverride* override_entry = find_override(overrides, record);
        const auto rule_match = override_entry ? std::nullopt : match_rule(rules, record);
        const OrganizeRule* rule = rule_match ? &(*rule_match) : nullptr;

        std::string content_dir;
        std::string asset_id;
        std::string asset_type;

        if (override_entry != nullptr) {
            content_dir = override_entry->content_dir;
            asset_id = override_entry->asset_id;
            asset_type = asset_type_from_kind(record.kind);
        } else {
            content_dir = resolve_content_dir(rule, record);
            asset_id = make_asset_id(rule, record, content_dir);
            asset_type = resolve_asset_type(rule, record);
        }

        asset_id = unique_asset_id(asset_id, record, used_ids, stats);

        const auto filename = record.slug + "." + record.output_format;
        const auto rel_path = (std::filesystem::path(content_dir) / filename).generic_string();
        const auto dst = options.output_root / rel_path;

        try {
            copy_asset_file(src, dst, options.dry_run);
        } catch (const std::exception& ex) {
            spdlog::error("Copy failed for {}: {}", record.legacy_ref, ex.what());
            ++stats.errors;
            continue;
        }

        ContentAsset asset;
        asset.id = asset_id;
        asset.type = asset_type;
        asset.path = rel_path;
        asset.legacy_refs = {record.legacy_ref};
        asset.original_name = record.original_name;
        asset.slug = record.slug;
        asset.archive = record.archive;
        asset.virtual_path = record.virtual_path;
        asset.output_format = record.output_format;
        asset.size_bytes = record.size_bytes;
        assets.push_back(std::move(asset));

        ++stats.organized;
        if (content_dir.rfind("_unclassified", 0) == 0) {
            ++stats.unclassified;
        }
    }

    std::sort(assets.begin(), assets.end(),
              [](const ContentAsset& a, const ContentAsset& b) { return a.id < b.id; });

    write_content_manifest(options, assets, stats);
    write_aliases(options, assets);
    return stats;
}

} // namespace d2df::tools
