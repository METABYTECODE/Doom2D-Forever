#include <d2df/engine/render/tile_map_render_index.hpp>

#include <d2df/engine/map/tile_flags.hpp>

namespace d2df::engine::render {
namespace {

constexpr std::uint32_t kLayerOrder[] = {
    map::TILE_BACK,
    map::TILE_STEP,
    map::TILE_WALL,
    map::TILE_CLOSE_DOOR,
    map::TILE_OPEN_DOOR,
    map::TILE_ACID1,
    map::TILE_ACID2,
    map::TILE_WATER,
    map::TILE_FORE,
};

} // namespace

void TileMapRenderIndex::build(const map::TileMap& tile_map) {
    clear();

    for (std::size_t layer_index = 0; layer_index < tile_map.layers.size(); ++layer_index) {
        const auto& layer = tile_map.layers[layer_index];
        for (std::size_t tile_index = 0; tile_index < layer.tiles.size(); ++tile_index) {
            const auto& tile = layer.tiles[tile_index];
            if ((tile.visibility & map::TILE_VIS_HIDDEN) != 0) {
                continue;
            }

            for (std::size_t pass = 0; pass < kLayerCount; ++pass) {
                const std::uint32_t mask = kLayerOrder[pass];
                if ((tile.flags & mask) == 0) {
                    continue;
                }
                layer_tiles_[pass].push_back(TileDrawRef{layer_index, tile_index});
            }
        }
    }
}

void TileMapRenderIndex::clear() {
    for (auto& layer : layer_tiles_) {
        layer.clear();
    }
}

const std::vector<TileDrawRef>& TileMapRenderIndex::layer_tiles(std::size_t layer_index) const {
    return layer_tiles_[layer_index];
}

std::uint32_t TileMapRenderIndex::layer_mask(std::size_t layer_index) {
    return kLayerOrder[layer_index];
}

} // namespace d2df::engine::render
