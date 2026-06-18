#include <rivet/game/content/content_paths.hpp>

#include <SDL.h>

namespace rivet::game::content {

namespace {

[[nodiscard]] bool looks_like_assets_root(const std::filesystem::path& assets) {
    return std::filesystem::exists(assets / "levels")
        || std::filesystem::exists(assets / "tilesets");
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

std::filesystem::path tilesets_dir(const std::filesystem::path& assets_root) {
    return assets_root / "tilesets";
}

} // namespace rivet::game::content
