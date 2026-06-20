#include <rivet/physics/fluid_grid.hpp>

#include <algorithm>
#include <array>
#include <cmath>

namespace rivet::physics {

namespace {

[[nodiscard]] int cell_index(const int x, const int y, const int width) {
    return y * width + x;
}

[[nodiscard]] float intersection_area(const Aabb& a, const Aabb& b) {
    const float x0 = std::max(a.x, b.x);
    const float y0 = std::max(a.y, b.y);
    const float x1 = std::min(a.x + a.width, b.x + b.width);
    const float y1 = std::min(a.y + a.height, b.y + b.height);
    if (x1 <= x0 || y1 <= y0) {
        return 0.0f;
    }
    return (x1 - x0) * (y1 - y0);
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
    if (cells_.empty() || box.width <= 0.0f || box.height <= 0.0f) {
        return {};
    }

    const int x0 = std::max(0, static_cast<int>(std::floor(box.x / cell_size_)));
    const int y0 = std::max(0, static_cast<int>(std::floor(box.y / cell_size_)));
    const int x1 = std::min(
        width_ - 1,
        static_cast<int>(std::floor((box.x + box.width) / cell_size_)));
    const int y1 = std::min(
        height_ - 1,
        static_cast<int>(std::floor((box.y + box.height) / cell_size_)));

    const float body_area = box.width * box.height;
    std::array<float, 4> fluid_areas{};

    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            const Aabb cell{
                .x = static_cast<float>(x) * cell_size_,
                .y = static_cast<float>(y) * cell_size_,
                .width = cell_size_,
                .height = cell_size_,
            };
            const float overlap = intersection_area(box, cell);
            if (overlap <= 0.0f) {
                continue;
            }

            const std::uint8_t fluid = at_cell(x, y);
            if (fluid < fluid_areas.size()) {
                fluid_areas[fluid] += overlap;
            }
        }
    }

    std::uint8_t dominant = 0;
    float dominant_area = 0.0f;
    float total_fluid_area = 0.0f;
    for (std::uint8_t id = 1; id < fluid_areas.size(); ++id) {
        total_fluid_area += fluid_areas[id];
        if (fluid_areas[id] > dominant_area) {
            dominant = id;
            dominant_area = fluid_areas[id];
        }
    }

    const float immersion = std::clamp(total_fluid_area / body_area, 0.0f, 1.0f);
    return {
        .id = dominant,
        .immersed = dominant > 0 && immersion > 0.02f,
        .immersion = immersion,
    };
}

} // namespace rivet::physics
