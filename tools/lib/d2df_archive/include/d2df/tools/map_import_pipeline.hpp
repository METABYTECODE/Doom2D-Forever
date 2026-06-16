#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace d2df::tools {

struct MapImportOptions {
    std::filesystem::path content_root = "assets/content";
    bool dry_run = false;
};

struct MapImportStats {
    std::size_t maps_found = 0;
    std::size_t converted = 0;
    std::size_t failed = 0;
    std::size_t unresolved_refs = 0;
};

struct MapImportIssue {
    std::string map_id;
    std::string stage;
    std::string message;
};

[[nodiscard]] MapImportStats run_map_import(const MapImportOptions& options,
                                            std::vector<MapImportIssue>& issues);

} // namespace d2df::tools
