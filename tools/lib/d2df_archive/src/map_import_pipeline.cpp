#include <d2df/tools/map_import_pipeline.hpp>

#include <fstream>
#include <iostream>
#include <sstream>

#include <spdlog/spdlog.h>

#include <d2df/resources/content_catalog.hpp>
#include <d2df/tools/json_util.hpp>
#include <d2df/tools/legacy_map.hpp>
#include <d2df/tools/map_json.hpp>

namespace d2df::tools {
namespace {

std::filesystem::path json_output_path(const d2df::resources::ContentAssetInfo& asset) {
    std::filesystem::path legacy_path(asset.path);
    legacy_path.replace_extension(".json");
    const auto legacy_pos = legacy_path.generic_string().find("maps/legacy/");
    if (legacy_pos != std::string::npos) {
        return std::filesystem::path("maps") /
               legacy_path.generic_string().substr(legacy_pos + std::string("maps/legacy/").size());
    }
    return legacy_path.replace_filename(legacy_path.stem().string() + ".json");
}

void write_import_report(const MapImportOptions& options, const MapImportStats& stats,
                         const std::vector<MapImportIssue>& issues) {
    const auto report_path = options.content_root / "maps" / "import_report.json";
    std::ofstream out(report_path, std::ios::binary);
    if (!out) {
        spdlog::warn("Failed to write import report: {}", report_path.string());
        return;
    }

    out << "{\n";
    out << "  \"version\": 1,\n";
    out << "  \"generator\": \"d2df-map-import\",\n";
    out << "  \"stats\": {";
    out << "\"maps_found\":" << stats.maps_found << ",\"converted\":" << stats.converted
        << ",\"failed\":" << stats.failed << ",\"unresolved_refs\":" << stats.unresolved_refs
        << "},\n";
    out << "  \"issues\": [\n";
    for (std::size_t i = 0; i < issues.size(); ++i) {
        const auto& issue = issues[i];
        out << "    {\"map_id\":\"" << json_escape(issue.map_id) << "\",\"stage\":\""
            << json_escape(issue.stage) << "\",\"message\":\"" << json_escape(issue.message)
            << "\"}";
        if (i + 1 < issues.size()) {
            out << ',';
        }
        out << '\n';
    }
    out << "  ]\n}\n";
    spdlog::info("Wrote {}", report_path.string());
}

void write_import_summary(const MapImportOptions& options, const MapImportStats& stats,
                          const std::vector<MapImportIssue>& issues) {
    std::ostringstream text;
    text << "========================================\n";
    text << "  d2df-map-import summary\n";
    text << "========================================\n";
    text << "Content:       " << options.content_root.generic_string() << '\n';
    text << "Maps found:    " << stats.maps_found << '\n';
    text << "Converted:     " << stats.converted << '\n';
    text << "Failed:        " << stats.failed << '\n';
    text << "Unresolved refs:" << stats.unresolved_refs << '\n';
    if (!issues.empty()) {
        text << "\n--- Problems (" << issues.size() << ") ---\n";
        for (std::size_t i = 0; i < issues.size() && i < 30; ++i) {
            text << '[' << issues[i].stage << "] " << issues[i].map_id << "\n    "
                 << issues[i].message << '\n';
        }
        if (issues.size() > 30) {
            text << "... see maps/import_report.json\n";
        }
    } else {
        text << "\nNo problems.\n";
    }
    text << "========================================\n";

    const auto summary = text.str();
    std::cout << summary;

    if (options.dry_run) {
        return;
    }

    const auto summary_path = options.content_root / "maps" / "import_summary.txt";
    std::ofstream out(summary_path, std::ios::binary);
    if (out) {
        out << summary;
    }
}

} // namespace

MapImportStats run_map_import(const MapImportOptions& options, std::vector<MapImportIssue>& issues) {
    MapImportStats stats;

    d2df::resources::ContentCatalog catalog;
    catalog.load(options.content_root);

    for (const auto& asset : catalog.assets()) {
        if (asset.type != "map") {
            continue;
        }
        ++stats.maps_found;

        const auto source_path = options.content_root / asset.path;
        spdlog::info("[{}/{}] {}", stats.converted + stats.failed + 1, stats.maps_found,
                     asset.id);

        try {
            std::ifstream in(source_path, std::ios::binary);
            if (!in) {
                throw std::runtime_error("Failed to open map: " + source_path.string());
            }

            in.seekg(0, std::ios::end);
            const auto file_size = in.tellg();
            if (file_size <= 0) {
                throw std::runtime_error("Empty map file");
            }

            std::vector<std::uint8_t> bytes(static_cast<std::size_t>(file_size));
            in.seekg(0, std::ios::beg);
            in.read(reinterpret_cast<char*>(bytes.data()), file_size);
            if (!in) {
                throw std::runtime_error("Failed to read map file");
            }

            const auto document = parse_legacy_mapbin(bytes);
            std::vector<MapRefIssue> ref_issues;
            MapJsonOptions json_opts;
            json_opts.asset_id = asset.id;
            json_opts.legacy_ref = asset.legacy_refs.empty() ? "" : asset.legacy_refs.front();
            json_opts.source_path = asset.path;
            const auto json = write_map_json_v1(document, catalog, json_opts, ref_issues);
            stats.unresolved_refs += ref_issues.size();

            const auto output_rel = json_output_path(asset);
            const auto output_path = options.content_root / output_rel;
            if (!options.dry_run) {
                std::filesystem::create_directories(output_path.parent_path());
                std::ofstream out(output_path, std::ios::binary);
                if (!out) {
                    throw std::runtime_error("Failed to write JSON map: " + output_path.string());
                }
                out << json;
            }

            ++stats.converted;
            spdlog::info("  -> {}", output_rel.generic_string());
        } catch (const std::exception& ex) {
            ++stats.failed;
            issues.push_back({asset.id, "import", ex.what()});
            spdlog::error("  {}", ex.what());
        }
    }

    if (!options.dry_run) {
        write_import_report(options, stats, issues);
    }
    write_import_summary(options, stats, issues);
    return stats;
}

} // namespace d2df::tools
