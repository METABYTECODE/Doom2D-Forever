#pragma once

#include <d2df/engine/map/tile_map.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace d2df::engine::render {

struct TileDrawRef {
    std::size_t layer_index = 0;
    std::size_t tile_index = 0;
};

/// Precomputed tile draw order per render pass (mirrors legacy panel layers).
class TileMapRenderIndex {
public:
    static constexpr std::size_t kLayerCount = 9;

    void build(const map::TileMap& tile_map);
    void clear();

    [[nodiscard]] const std::vector<TileDrawRef>& layer_tiles(std::size_t layer_index) const;
    [[nodiscard]] static std::uint32_t layer_mask(std::size_t layer_index);

private:
    std::array<std::vector<TileDrawRef>, kLayerCount> layer_tiles_;
};

} // namespace d2df::engine::render
