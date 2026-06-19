#pragma once

#include <cstdint>
#include <vector>

#include <rivet/physics/aabb.hpp>

namespace rivet::physics {

/// Per-cell fluid volume grid. Engine reports overlap only; gameplay interprets fluid ids.
class FluidGrid {
public:
    FluidGrid() = default;
    FluidGrid(int width, int height, float cell_size, std::vector<std::uint8_t> fluids);

    [[nodiscard]] static FluidGrid from_grid(
        const std::vector<std::vector<int>>& cells,
        float cell_size);

    [[nodiscard]] bool empty() const { return cells_.empty(); }
    [[nodiscard]] int width() const { return width_; }
    [[nodiscard]] int height() const { return height_; }
    [[nodiscard]] float cell_size() const { return cell_size_; }
    [[nodiscard]] std::uint8_t at_cell(int x, int y) const;

    struct Sample {
        std::uint8_t id = 0;
        bool immersed = false;
    };

    /// Dominant fluid id under the lower 2/3 of the box (platformer body probe).
    [[nodiscard]] Sample sample_aabb(const Aabb& box) const;

private:
    int width_ = 0;
    int height_ = 0;
    float cell_size_ = 8.0f;
    std::vector<std::uint8_t> cells_;
};

} // namespace rivet::physics
