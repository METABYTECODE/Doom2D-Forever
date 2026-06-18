#include <d2df/game/game_map_bundle.hpp>

#include <d2df/engine/map/legacy_map_adapter.hpp>
#include <d2df/map/map_json_loader.hpp>

namespace d2df::game {

GameMapBundle load_legacy_map(const std::filesystem::path& path) {
    GameMapBundle bundle;
    bundle.legacy = map::load_map_json_v1(path);
    bundle.runtime = engine::map::tile_map_from_legacy(bundle.legacy);
    return bundle;
}

} // namespace d2df::game
