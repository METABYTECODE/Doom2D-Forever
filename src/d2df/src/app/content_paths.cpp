#include <d2df/app/content_paths.hpp>

#include <SDL.h>

namespace d2df {
namespace {

std::optional<std::filesystem::path> find_content_root_from(const std::filesystem::path& start) {
    auto dir = start;
    for (int depth = 0; depth < 8; ++depth) {
        const auto manifest = dir / "assets" / "content" / "manifest.json";
        if (std::filesystem::exists(manifest)) {
            return dir / "assets" / "content";
        }
        if (!dir.has_parent_path() || dir.parent_path() == dir) {
            break;
        }
        dir = dir.parent_path();
    }
    return std::nullopt;
}

} // namespace

std::optional<ResolvedContentPaths> resolve_content_paths(const std::filesystem::path& map_rel) {
    if (const auto cwd_root = find_content_root_from(std::filesystem::current_path())) {
        ResolvedContentPaths paths;
        paths.content_root = *cwd_root;
        paths.map_path = *cwd_root / map_rel;
        return paths;
    }

    char* base_path = SDL_GetBasePath();
    if (base_path != nullptr) {
        const std::filesystem::path exe_dir(base_path);
        SDL_free(base_path);
        if (const auto exe_root = find_content_root_from(exe_dir)) {
            ResolvedContentPaths paths;
            paths.content_root = *exe_root;
            paths.map_path = *exe_root / map_rel;
            return paths;
        }
    }

    return std::nullopt;
}

} // namespace d2df
