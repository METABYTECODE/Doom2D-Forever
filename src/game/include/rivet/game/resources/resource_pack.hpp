#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace rivet::game::resources {

/// A loadable resource pack (`assets/resourcepacks/{id}/` + `pack.json`).
class ResourcePack {
public:
    struct Manifest {
        std::string id;
        std::string name;
        int version = 1;
    };

    [[nodiscard]] static std::optional<std::filesystem::path> packs_root(
        const std::filesystem::path& assets_root);
    [[nodiscard]] static std::optional<std::filesystem::path> pack_dir(
        const std::filesystem::path& assets_root,
        const std::string& pack_id);
    [[nodiscard]] static std::optional<ResourcePack> load(
        const std::filesystem::path& assets_root,
        const std::string& pack_id);

    [[nodiscard]] const Manifest& manifest() const { return manifest_; }
    [[nodiscard]] const std::filesystem::path& root() const { return root_; }

    [[nodiscard]] std::filesystem::path tilesets_dir() const;
    [[nodiscard]] std::optional<std::filesystem::path> resolve_background(const std::string& asset_id) const;
    [[nodiscard]] std::optional<std::filesystem::path> resolve_music(const std::string& asset_id) const;

private:
    ResourcePack(Manifest manifest, std::filesystem::path root);

    [[nodiscard]] std::optional<std::filesystem::path> resolve_catalog_asset(
        const std::filesystem::path& catalog_dir,
        const std::string& asset_id,
        const char* const* extensions,
        std::size_t extension_count) const;

    Manifest manifest_{};
    std::filesystem::path root_;
};

[[nodiscard]] std::optional<std::filesystem::path> resolve_assets_root();
inline constexpr const char* kDefaultPackId = "dev";

} // namespace rivet::game::resources
