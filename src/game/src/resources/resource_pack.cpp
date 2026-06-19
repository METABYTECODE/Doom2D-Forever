#include <rivet/game/resources/resource_pack.hpp>

#include <fstream>
#include <stdexcept>

#include <SDL.h>
#include <nlohmann/json.hpp>

namespace rivet::game::resources {

namespace {

[[nodiscard]] bool looks_like_assets_root(const std::filesystem::path& assets) {
    return std::filesystem::exists(assets / "levels")
        || std::filesystem::exists(assets / "resourcepacks");
}

[[nodiscard]] std::optional<std::filesystem::path> find_assets_root_from(const std::filesystem::path& start) {
    auto dir = start;
    for (int depth = 0; depth < 10; ++depth) {
        const auto assets = dir / "assets";
        if (looks_like_assets_root(assets)) {
            return assets;
        }
        if (!dir.has_parent_path() || dir.parent_path() == dir) {
            break;
        }
        dir = dir.parent_path();
    }
    return std::nullopt;
}

[[nodiscard]] bool has_allowed_extension(
    const std::filesystem::path& path,
    const char* const* extensions,
    const std::size_t extension_count) {
    const auto ext = path.extension().string();
    for (std::size_t i = 0; i < extension_count; ++i) {
        if (ext == extensions[i]) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] std::string normalize_asset_id(std::string asset_id) {
    while (!asset_id.empty() && (asset_id.front() == '/' || asset_id.front() == '\\')) {
        asset_id.erase(asset_id.begin());
    }
    return asset_id;
}

} // namespace

std::optional<std::filesystem::path> resolve_assets_root() {
    if (const auto cwd_root = find_assets_root_from(std::filesystem::current_path())) {
        return cwd_root;
    }

    char* base_path = SDL_GetBasePath();
    if (base_path != nullptr) {
        const std::filesystem::path exe_dir(base_path);
        SDL_free(base_path);
        if (const auto exe_root = find_assets_root_from(exe_dir)) {
            return exe_root;
        }
    }

    const auto fallback = std::filesystem::path("assets");
    if (looks_like_assets_root(fallback)) {
        return fallback;
    }
    return std::nullopt;
}

std::optional<std::filesystem::path> ResourcePack::packs_root(const std::filesystem::path& assets_root) {
    const auto root = assets_root / "resourcepacks";
    if (!std::filesystem::exists(root)) {
        return std::nullopt;
    }
    return root;
}

std::optional<std::filesystem::path> ResourcePack::pack_dir(
    const std::filesystem::path& assets_root,
    const std::string& pack_id) {
    const auto packs = packs_root(assets_root);
    if (!packs) {
        return std::nullopt;
    }
    const auto dir = *packs / pack_id;
    if (!std::filesystem::exists(dir / "pack.json")) {
        return std::nullopt;
    }
    return dir;
}

ResourcePack::ResourcePack(Manifest manifest, std::filesystem::path root)
    : manifest_(std::move(manifest))
    , root_(std::move(root)) {}

std::optional<ResourcePack> ResourcePack::load(
    const std::filesystem::path& assets_root,
    const std::string& pack_id) {
    const auto dir = pack_dir(assets_root, pack_id);
    if (!dir) {
        return std::nullopt;
    }

    std::ifstream input(*dir / "pack.json");
    if (!input) {
        return std::nullopt;
    }

    const auto json = nlohmann::json::parse(input);
    Manifest manifest;
    manifest.id = json.value("id", pack_id);
    manifest.name = json.value("name", pack_id);
    manifest.version = json.value("version", 1);
    return ResourcePack(std::move(manifest), *dir);
}

std::filesystem::path ResourcePack::tilesets_dir() const {
    return root_ / "textures" / "tilesets";
}

std::optional<std::filesystem::path> ResourcePack::resolve_catalog_asset(
    const std::filesystem::path& catalog_dir,
    const std::string& asset_id,
    const char* const* extensions,
    const std::size_t extension_count) const {
    const std::string id = normalize_asset_id(asset_id);
    if (id.empty()) {
        return std::nullopt;
    }

    const auto direct = root_ / id;
    if (std::filesystem::exists(direct) && direct.has_extension()) {
        return direct;
    }

    const auto legacy = root_ / catalog_dir / id;
    if (std::filesystem::exists(legacy) && legacy.has_extension()) {
        return legacy;
    }

    for (std::size_t i = 0; i < extension_count; ++i) {
        const auto candidate = root_ / catalog_dir / (id + extensions[i]);
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    if (!std::filesystem::exists(root_ / catalog_dir)) {
        return std::nullopt;
    }

    const auto target_stem = std::filesystem::path(id).filename().string();
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root_ / catalog_dir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (!has_allowed_extension(entry.path(), extensions, extension_count)) {
            continue;
        }

        const auto relative = std::filesystem::relative(entry.path(), root_ / catalog_dir);
        std::string relative_id = relative.generic_string();
        const auto dot = relative_id.find_last_of('.');
        if (dot != std::string::npos) {
            relative_id.erase(dot);
        }
        if (relative_id == id || entry.path().stem().string() == target_stem) {
            return entry.path();
        }
    }

    return std::nullopt;
}

std::optional<std::filesystem::path> ResourcePack::resolve_background(const std::string& asset_id) const {
    static constexpr const char* kExtensions[] = {".png", ".jpg", ".jpeg", ".webp", ".gif"};
    return resolve_catalog_asset(
        std::filesystem::path("textures") / "backgrounds",
        asset_id,
        kExtensions,
        sizeof(kExtensions) / sizeof(kExtensions[0]));
}

std::optional<std::filesystem::path> ResourcePack::resolve_music(const std::string& asset_id) const {
    static constexpr const char* kExtensions[] = {".ogg", ".wav", ".mp3", ".mid", ".midi"};
    return resolve_catalog_asset(
        std::filesystem::path("audio") / "music",
        asset_id,
        kExtensions,
        sizeof(kExtensions) / sizeof(kExtensions[0]));
}

std::optional<std::filesystem::path> ResourcePack::resolve_sfx(const std::string& asset_id) const {
    static constexpr const char* kExtensions[] = {".ogg", ".wav", ".mp3"};
    return resolve_catalog_asset(
        std::filesystem::path("audio") / "sfx",
        asset_id,
        kExtensions,
        sizeof(kExtensions) / sizeof(kExtensions[0]));
}

std::optional<std::filesystem::path> ResourcePack::resolve_model(const std::string& model_id) const {
    const std::string id = normalize_asset_id(model_id);
    if (id.empty()) {
        return std::nullopt;
    }

    const auto direct = root_ / "models" / (id + ".model.json");
    if (std::filesystem::exists(direct)) {
        return direct;
    }

    const auto stem = std::filesystem::path(id).stem().string();
    const auto by_stem = root_ / "models" / (stem + ".model.json");
    if (std::filesystem::exists(by_stem)) {
        return by_stem;
    }

    return std::nullopt;
}

std::optional<std::filesystem::path> ResourcePack::resolve_sprite_atlas(const std::string& asset_id) const {
    static constexpr const char* kExtensions[] = {".png", ".jpg", ".jpeg", ".webp", ".gif"};
    return resolve_catalog_asset(
        std::filesystem::path("textures") / "sprites",
        asset_id,
        kExtensions,
        sizeof(kExtensions) / sizeof(kExtensions[0]));
}

} // namespace rivet::game::resources
