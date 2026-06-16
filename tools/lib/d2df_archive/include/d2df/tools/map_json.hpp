#pragma once

#include <d2df/resources/content_catalog.hpp>
#include <d2df/tools/legacy_map.hpp>

#include <string>
#include <vector>

namespace d2df::tools {

struct MapRefIssue {
    std::string field;
    std::string legacy_ref;
    std::string message;
};

struct MapJsonOptions {
    std::string asset_id;
    std::string legacy_ref;
    std::string source_path;
};

[[nodiscard]] std::string write_map_json_v1(const LegacyMapDocument& document,
                                            const d2df::resources::ContentCatalog& catalog,
                                            const MapJsonOptions& options,
                                            std::vector<MapRefIssue>& issues);

} // namespace d2df::tools
