#include <rivet/physics/tile_collider.hpp>

#include <algorithm>
#include <cmath>

namespace rivet::physics {

namespace {

[[nodiscard]] int cell_index(const int x, const int y, const int width) {
    return y * width + x;
}

} // namespace

TileCollider::TileCollider(
    const int width,
    const int height,
    const float cell_size,
    std::vector<std::uint8_t> solid)
    : width_(width)
    , height_(height)
    , cell_size_(cell_size)
    , cells_(std::move(solid)) {}

TileCollider TileCollider::from_grid(const std::vector<std::vector<int>>& cells, const float cell_size) {
    if (cells.empty()) {
        return {};
    }

    const int height = static_cast<int>(cells.size());
    const int width = static_cast<int>(cells.front().size());
    std::vector<std::uint8_t> solid(static_cast<std::size_t>(width * height), 0);
    for (int y = 0; y < height; ++y) {
        const auto& row = cells[static_cast<std::size_t>(y)];
        for (int x = 0; x < width; ++x) {
            if (x < static_cast<int>(row.size()) && row[static_cast<std::size_t>(x)] != 0) {
                solid[static_cast<std::size_t>(cell_index(x, y, width))] = 1;
            }
        }
    }

    return TileCollider(width, height, cell_size, std::move(solid));
}

bool TileCollider::is_solid(const int x, const int y) const {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) {
        return false;
    }
    return cells_[static_cast<std::size_t>(cell_index(x, y, width_))] != 0;
}

Aabb TileCollider::cell_aabb(const int x, const int y) const {
    return {
        .x = static_cast<float>(x) * cell_size_,
        .y = static_cast<float>(y) * cell_size_,
        .width = cell_size_,
        .height = cell_size_,
    };
}

void TileCollider::for_each_cell_in_aabb(
    const Aabb& box,
    const std::function<void(int x, int y, const Aabb& cell)>& fn) const {
    if (cells_.empty() || fn == nullptr) {
        return;
    }

    const int x0 = std::max(0, static_cast<int>(std::floor(box.x / cell_size_)));
    const int y0 = std::max(0, static_cast<int>(std::floor(box.y / cell_size_)));
    const int x1 = std::min(
        width_ - 1,
        static_cast<int>(std::floor((box.x + box.width) / cell_size_)));
    const int y1 = std::min(
        height_ - 1,
        static_cast<int>(std::floor((box.y + box.height) / cell_size_)));

    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            if (!is_solid(x, y)) {
                continue;
            }
            fn(x, y, cell_aabb(x, y));
        }
    }
}

} // namespace rivet::physics
