#include <rivet/physics/spatial_grid.hpp>

#include <cmath>

#include <rivet/ecs/components/collider.hpp>
#include <rivet/ecs/components/transform.hpp>

namespace rivet::physics {

std::int64_t SpatialGrid::cell_key(const int x, const int y) const {
    return (static_cast<std::int64_t>(x) << 32) ^ static_cast<std::uint32_t>(y);
}

void SpatialGrid::build(const rivet::ecs::World& world, const float cell_size) {
    cell_size_ = std::max(8.0f, cell_size);
    cells_.clear();

    const auto view = world.registry().view<rivet::ecs::components::Transform, rivet::ecs::components::Collider>();
    for (const auto entity : view) {
        const auto& collider = view.get<rivet::ecs::components::Collider>(entity);
        if (!collider.is_static) {
            continue;
        }
        const auto& transform = view.get<rivet::ecs::components::Transform>(entity);
        const Aabb box = make_aabb(transform, collider);

        const int x0 = static_cast<int>(std::floor(box.x / cell_size_));
        const int y0 = static_cast<int>(std::floor(box.y / cell_size_));
        const int x1 = static_cast<int>(std::floor((box.x + box.width) / cell_size_));
        const int y1 = static_cast<int>(std::floor((box.y + box.height) / cell_size_));

        for (int y = y0; y <= y1; ++y) {
            for (int x = x0; x <= x1; ++x) {
                cells_[cell_key(x, y)].push_back(entity);
            }
        }
    }
}

void SpatialGrid::for_each_in_aabb(
    const Aabb& box,
    const std::function<void(entt::entity entity, const Aabb& query)>& fn) const {
    if (fn == nullptr || cells_.empty()) {
        return;
    }

    const int x0 = static_cast<int>(std::floor(box.x / cell_size_));
    const int y0 = static_cast<int>(std::floor(box.y / cell_size_));
    const int x1 = static_cast<int>(std::floor((box.x + box.width) / cell_size_));
    const int y1 = static_cast<int>(std::floor((box.y + box.height) / cell_size_));

    std::vector<entt::entity> seen;
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            const auto it = cells_.find(cell_key(x, y));
            if (it == cells_.end()) {
                continue;
            }
            for (const auto entity : it->second) {
                if (std::find(seen.begin(), seen.end(), entity) != seen.end()) {
                    continue;
                }
                seen.push_back(entity);
            }
        }
    }

    for (const auto entity : seen) {
        fn(entity, box);
    }
}

} // namespace rivet::physics
