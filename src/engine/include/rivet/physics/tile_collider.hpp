#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include <rivet/physics/aabb.hpp>

namespace rivet::physics {

/// Solid tile grid for level collision (engine-owned representation).
class TileCollider {
public:
    TileCollider() = default;
    TileCollider(int width, int height, float cell_size, std::vector<std::uint8_t> solid);

    [[nodiscard]] static TileCollider from_grid(
        const std::vector<std::vector<int>>& cells,
        float cell_size);

    [[nodiscard]] bool empty() const { return cells_.empty(); }
    [[nodiscard]] int width() const { return width_; }
    [[nodiscard]] int height() const { return height_; }
    [[nodiscard]] float cell_size() const { return cell_size_; }
    [[nodiscard]] bool is_solid(int x, int y) const;
    [[nodiscard]] Aabb cell_aabb(int x, int y) const;

    void for_each_cell_in_aabb(const Aabb& box, const std::function<void(int x, int y, const Aabb& cell)>& fn) const;

private:
    int width_ = 0;
    int height_ = 0;
    float cell_size_ = 8.0f;
    std::vector<std::uint8_t> cells_;
};

} // namespace rivet::physics
