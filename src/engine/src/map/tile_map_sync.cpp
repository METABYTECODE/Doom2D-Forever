#include <d2df/engine/map/tile_map_sync.hpp>

namespace d2df::engine::map {
namespace {

constexpr const char* kWorldLayerName = "legacy_panels";

TileLayer* find_world_layer(TileMap& tile_map) {
    for (auto& layer : tile_map.layers) {
        if (layer.name == kWorldLayerName) {
            return &layer;
        }
    }
    if (!tile_map.layers.empty()) {
        return &tile_map.layers.front();
    }
    return nullptr;
}

TilePlacement panel_to_tile(const ::d2df::map::MapPanel& panel) {
    TilePlacement tile;
    tile.x = panel.position.x;
    tile.y = panel.position.y;
    tile.width = panel.size.width;
    tile.height = panel.size.height;
    tile.tile_index = panel.texture;
    tile.flags = panel.type;
    tile.visibility = panel.flags;
    tile.alpha = panel.alpha;
    tile.enabled = panel.enabled;
    return tile;
}

void rebuild_world_layer(TileMap& tile_map, const std::vector<::d2df::map::MapPanel>& panels) {
    TileLayer layer;
    layer.name = kWorldLayerName;
    layer.tiles.reserve(panels.size());
    for (const auto& panel : panels) {
        layer.tiles.push_back(panel_to_tile(panel));
    }

    for (auto& existing : tile_map.layers) {
        if (existing.name == kWorldLayerName) {
            existing = std::move(layer);
            return;
        }
    }
    tile_map.layers.insert(tile_map.layers.begin(), std::move(layer));
}

} // namespace

void sync_tiles_from_panels(TileMap& tile_map, const std::vector<::d2df::map::MapPanel>& panels) {
    TileLayer* layer = find_world_layer(tile_map);
    if (layer == nullptr || layer->tiles.size() != panels.size()) {
        rebuild_world_layer(tile_map, panels);
        return;
    }

    for (std::size_t i = 0; i < panels.size(); ++i) {
        auto& tile = layer->tiles[i];
        const auto& panel = panels[i];
        tile.x = panel.position.x;
        tile.y = panel.position.y;
        tile.width = panel.size.width;
        tile.height = panel.size.height;
        tile.tile_index = panel.texture;
        tile.flags = panel.type;
        tile.visibility = panel.flags;
        tile.alpha = panel.alpha;
        tile.enabled = panel.enabled;
    }
}

} // namespace d2df::engine::map
