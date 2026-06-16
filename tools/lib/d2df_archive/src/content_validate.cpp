#include <d2df/tools/content_validate.hpp>

#include <fstream>
#include <iostream>
#include <sstream>

#include <spdlog/spdlog.h>

#include <d2df/resources/content_catalog.hpp>
#include <d2df/tools/json_util.hpp>
#include <d2df/resources/legacy_ref.hpp>

namespace d2df::tools {
namespace {

void validate_asset_file(const d2df::resources::ContentCatalog& catalog,
                         const d2df::resources::ContentAssetInfo& asset,
                         ContentValidateStats& stats, std::vector<ContentValidateIssue>& issues) {
    const auto path = catalog.content_root() / asset.path;
    if (!std::filesystem::exists(path)) {
        ++stats.missing_files;
        issues.push_back({asset.id, "path", "missing file: " + asset.path});
    }
}

void validate_map_json(const d2df::resources::ContentCatalog& catalog, const std::filesystem::path& path,
                       ContentValidateStats& stats, std::vector<ContentValidateIssue>& issues) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        ++stats.broken_refs;
        issues.push_back({path.filename().string(), "file", "failed to open map json"});
        return;
    }

    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ++stats.maps_checked;

    const auto check_ref = [&](std::string_view key) {
        if (const auto value = json_extract_string(content, key)) {
            if (d2df::resources::is_builtin_texture_ref(*value)) {
                return;
            }
            if (value->find(':') != std::string::npos && !catalog.resolve_alias(*value)) {
                ++stats.broken_refs;
                issues.push_back({path.filename().string(), std::string(key),
                                  "unresolved legacy ref: " + *value});
            } else if (value->find(':') == std::string::npos && catalog.find_asset(*value) == nullptr) {
                ++stats.broken_refs;
                issues.push_back({path.filename().string(), std::string(key),
                                  "unknown asset id: " + *value});
            }
        }
    };

    check_ref("music");
    check_ref("sky");

    std::size_t pos = 0;
    while ((pos = content.find("\"path\":\"", pos)) != std::string::npos) {
        pos += 8;
        const auto end = content.find('"', pos);
        if (end == std::string::npos) {
            break;
        }
        const std::string ref = content.substr(pos, end - pos);
        if (d2df::resources::is_builtin_texture_ref(ref)) {
            pos = end + 1;
            continue;
        }
        if (ref.find(':') != std::string::npos && !catalog.resolve_alias(ref)) {
            ++stats.broken_refs;
            issues.push_back({path.filename().string(), "textures.path",
                              "unresolved legacy ref: " + ref});
        } else if (ref.find(':') == std::string::npos && catalog.find_asset(ref) == nullptr) {
            ++stats.broken_refs;
            issues.push_back({path.filename().string(), "textures.path", "unknown asset id: " + ref});
        }
        pos = end + 1;
    }
}

void write_validate_report(const ContentValidateOptions& options, const ContentValidateStats& stats,
                           const std::vector<ContentValidateIssue>& issues) {
    const auto report_path = options.content_root / "validation_report.json";
    std::ofstream out(report_path, std::ios::binary);
    if (!out) {
        spdlog::warn("Failed to write validation report");
        return;
    }

    out << "{\n  \"version\": 1,\n  \"stats\": {";
    out << "\"assets\":" << stats.assets << ",\"maps_checked\":" << stats.maps_checked
        << ",\"missing_files\":" << stats.missing_files
        << ",\"broken_refs\":" << stats.broken_refs << "},\n  \"issues\": [\n";
    for (std::size_t i = 0; i < issues.size(); ++i) {
        out << "    {\"asset_id\":\"" << json_escape(issues[i].asset_id) << "\",\"field\":\""
            << json_escape(issues[i].field) << "\",\"message\":\"" << json_escape(issues[i].message)
            << "\"}";
        if (i + 1 < issues.size()) {
            out << ',';
        }
        out << '\n';
    }
    out << "  ]\n}\n";
}

} // namespace

ContentValidateStats run_content_validate(const ContentValidateOptions& options,
                                          std::vector<ContentValidateIssue>& issues) {
    ContentValidateStats stats;

    d2df::resources::ContentCatalog catalog;
    catalog.load(options.content_root);
    stats.assets = catalog.assets().size();

    for (const auto& asset : catalog.assets()) {
        validate_asset_file(catalog, asset, stats, issues);
    }

    const auto maps_root = options.content_root / "maps";
    if (std::filesystem::exists(maps_root)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(maps_root)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            if (entry.path().extension() != ".json") {
                continue;
            }
            if (entry.path().filename() == "import_report.json") {
                continue;
            }
            validate_map_json(catalog, entry.path(), stats, issues);
        }
    }

    write_validate_report(options, stats, issues);

    std::cout << "Validated " << stats.assets << " assets, " << stats.maps_checked << " JSON maps\n";
    std::cout << "Missing files: " << stats.missing_files << ", broken refs: " << stats.broken_refs
              << '\n';
    return stats;
}

} // namespace d2df::tools
