#pragma once

#include <rivet/ecs/world.hpp>
#include <rivet/game/level/level_data.hpp>

namespace rivet::game::level {

struct LevelSpawnResult {
    rivet::ecs::Entity player = rivet::ecs::kNullEntity;
    float world_width = 0.0f;
    float world_height = 0.0f;
};

/// Builds ECS entities from a loaded level document.
[[nodiscard]] LevelSpawnResult spawn_level(rivet::ecs::World& world, const LevelData& level);

} // namespace rivet::game::level
