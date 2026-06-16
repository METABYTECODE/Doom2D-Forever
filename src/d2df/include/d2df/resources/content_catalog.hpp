#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace d2df::resources {

struct ContentAssetInfo {
    std::string id;
    std::string type;
    std::string path;
    std::vector<std::string> legacy_refs;
};

class ContentCatalog {
public:
    void load(const std::filesystem::path& content_root);

    [[nodiscard]] const std::filesystem::path& content_root() const { return content_root_; }

    [[nodiscard]] const ContentAssetInfo* find_asset(std::string_view id) const;

    [[nodiscard]] std::optional<std::string> resolve_alias(std::string_view legacy_ref) const;

    [[nodiscard]] std::filesystem::path asset_path(std::string_view id) const;

    [[nodiscard]] const std::vector<ContentAssetInfo>& assets() const { return assets_; }

private:
    void load_aliases(const std::filesystem::path& aliases_path);
    std::filesystem::path content_root_;
    std::vector<ContentAssetInfo> assets_;
    std::unordered_map<std::string, std::size_t> assets_by_id_;
    std::unordered_map<std::string, std::string> aliases_;
};

} // namespace d2df::resources
