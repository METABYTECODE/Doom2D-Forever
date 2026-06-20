#pragma once

#include <vector>

#include <rivet/ecs/world.hpp>
#include <rivet/game/level/level_data.hpp>
#include <rivet/game/resources/resource_pack.hpp>

namespace rivet::game::level {

struct LevelSpawnResult {
    rivet::ecs::Entity player = rivet::ecs::kNullEntity;
    std::vector<rivet::ecs::Entity> patrols;
    float world_width = 0.0f;
    float world_height = 0.0f;
};

struct LevelSpawnContext {
    const resources::ResourcePack* pack = nullptr;
};

/// Builds ECS entities from a loaded level document.
[[nodiscard]] LevelSpawnResult spawn_level(
    rivet::ecs::World& world,
    const LevelData& level,
    const LevelSpawnContext& context = {});

} // namespace rivet::game::level
