#include <d2df/engine/map/map_loader.hpp>

#include <fstream>
#include <stdexcept>

#include <d2df/engine/map/legacy_map_adapter.hpp>
#include <d2df/engine/map/tile_map_json.hpp>
#include <d2df/map/map_json_loader.hpp>

namespace d2df::engine::map {

TileMap load_map_file(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Failed to open map file: " + path.string());
    }

    std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (contents.empty()) {
        throw std::runtime_error("Empty map file: " + path.string());
    }

    if (contents.find("\"format\"") != std::string::npos &&
        contents.find(TileMap::kFormatId) != std::string::npos) {
        return load_tile_map_json_string(contents);
    }

    const auto legacy = ::d2df::map::load_map_json_v1(path);
    return tile_map_from_legacy(legacy);
}

} // namespace d2df::engine::map
