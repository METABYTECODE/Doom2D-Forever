#pragma once

#include <d2df/map/map_document.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace d2df::render {

/// Precomputed panel indices per render layer (built once per map load).
class MapRenderIndex {
public:
    static constexpr std::size_t kLayerCount = 9;

    void build(const map::MapDocument& document);
    void clear();

    [[nodiscard]] const std::vector<std::size_t>& layer_panels(std::size_t layer_index) const;
    [[nodiscard]] static std::uint16_t layer_mask(std::size_t layer_index);

private:
    std::array<std::vector<std::size_t>, kLayerCount> layer_panels_;
};

} // namespace d2df::render
