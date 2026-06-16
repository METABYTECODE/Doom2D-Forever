#pragma once

#include <d2df/resources/content_catalog.hpp>

#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace d2df::resources {

class AssetDatabase {
public:
    void load(const std::filesystem::path& content_root);

    [[nodiscard]] const ContentCatalog& catalog() const { return catalog_; }

    [[nodiscard]] const ContentAssetInfo* get(std::string_view asset_id) const;

    [[nodiscard]] std::optional<std::string> resolve_legacy(std::string_view legacy_ref) const;

    [[nodiscard]] std::filesystem::path path_for(std::string_view asset_id) const;

    [[nodiscard]] std::vector<std::uint8_t> read_bytes(std::string_view asset_id) const;

private:
    ContentCatalog catalog_;
};

} // namespace d2df::resources
