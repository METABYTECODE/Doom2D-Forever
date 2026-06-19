#include <rivet/physics/fluid_grid.hpp>

#include <algorithm>
#include <array>
#include <cmath>

namespace rivet::physics {

namespace {

[[nodiscard]] int cell_index(const int x, const int y, const int width) {
    return y * width + x;
}

[[nodiscard]] Aabb immersion_probe(const Aabb& box) {
    const float probe_height = box.height * (2.0f / 3.0f);
    return {
        .x = box.x,
        .y = box.y + (box.height - probe_height),
        .width = box.width,
        .height = probe_height,
    };
}

} // namespace

FluidGrid::FluidGrid(
    const int width,
    const int height,
    const float cell_size,
    std::vector<std::uint8_t> fluids)
    : width_(width)
    , height_(height)
    , cell_size_(cell_size)
    , cells_(std::move(fluids)) {}

FluidGrid FluidGrid::from_grid(const std::vector<std::vector<int>>& cells, const float cell_size) {
    if (cells.empty()) {
        return {};
    }

    const int height = static_cast<int>(cells.size());
    const int width = static_cast<int>(cells.front().size());
    std::vector<std::uint8_t> fluids(static_cast<std::size_t>(width * height), 0);
    for (int y = 0; y < height; ++y) {
        const auto& row = cells[static_cast<std::size_t>(y)];
        for (int x = 0; x < width; ++x) {
            if (x < static_cast<int>(row.size())) {
                const int value = row[static_cast<std::size_t>(x)];
                if (value > 0) {
                    fluids[static_cast<std::size_t>(cell_index(x, y, width))] =
                        static_cast<std::uint8_t>(value);
                }
            }
        }
    }

    return FluidGrid(width, height, cell_size, std::move(fluids));
}

std::uint8_t FluidGrid::at_cell(const int x, const int y) const {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) {
        return 0;
    }
    return cells_[static_cast<std::size_t>(cell_index(x, y, width_))];
}

FluidGrid::Sample FluidGrid::sample_aabb(const Aabb& box) const {
    if (cells_.empty()) {
        return {};
    }

    const Aabb probe = immersion_probe(box);
    const int x0 = std::max(0, static_cast<int>(std::floor(probe.x / cell_size_)));
    const int y0 = std::max(0, static_cast<int>(std::floor(probe.y / cell_size_)));
    const int x1 = std::min(
        width_ - 1,
        static_cast<int>(std::floor((probe.x + probe.width) / cell_size_)));
    const int y1 = std::min(
        height_ - 1,
        static_cast<int>(std::floor((probe.y + probe.height) / cell_size_)));

    std::array<int, 4> counts{};
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            const Aabb cell{
                .x = static_cast<float>(x) * cell_size_,
                .y = static_cast<float>(y) * cell_size_,
                .width = cell_size_,
                .height = cell_size_,
            };
            if (!probe.intersects(cell)) {
                continue;
            }

            const std::uint8_t fluid = at_cell(x, y);
            if (fluid < counts.size()) {
                ++counts[fluid];
            }
        }
    }

    std::uint8_t dominant = 0;
    int dominant_count = 0;
    for (std::uint8_t id = 1; id < counts.size(); ++id) {
        if (counts[id] > dominant_count) {
            dominant = id;
            dominant_count = counts[id];
        }
    }

    return {
        .id = dominant,
        .immersed = dominant_count > 0,
    };
}

} // namespace rivet::physics
