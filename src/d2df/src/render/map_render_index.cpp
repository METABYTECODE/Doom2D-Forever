#include <d2df/render/map_render_index.hpp>

#include <d2df/map/panel_types.hpp>

namespace d2df::render {
namespace {

constexpr std::uint16_t kLayerOrder[] = {
    map::PANEL_BACK,
    map::PANEL_STEP,
    map::PANEL_WALL,
    map::PANEL_CLOSEDOOR,
    map::PANEL_OPENDOOR,
    map::PANEL_ACID1,
    map::PANEL_ACID2,
    map::PANEL_WATER,
    map::PANEL_FORE,
};

} // namespace

void MapRenderIndex::build(const map::MapDocument& document) {
    clear();

    for (std::size_t panel_index = 0; panel_index < document.panels.size(); ++panel_index) {
        const map::MapPanel& panel = document.panels[panel_index];
        if ((panel.flags & map::PANEL_FLAG_HIDE) != 0) {
            continue;
        }

        for (std::size_t layer_index = 0; layer_index < kLayerCount; ++layer_index) {
            const std::uint16_t mask = kLayerOrder[layer_index];
            if ((panel.type & mask) != 0) {
                layer_panels_[layer_index].push_back(panel_index);
            }
        }
    }
}

void MapRenderIndex::clear() {
    for (auto& layer : layer_panels_) {
        layer.clear();
    }
}

const std::vector<std::size_t>& MapRenderIndex::layer_panels(std::size_t layer_index) const {
    return layer_panels_[layer_index];
}

std::uint16_t MapRenderIndex::layer_mask(std::size_t layer_index) {
    return kLayerOrder[layer_index];
}

} // namespace d2df::render
