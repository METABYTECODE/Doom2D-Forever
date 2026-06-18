#include <stdexcept>

#include <rivet/game/level/level_spawner.hpp>

#include <rivet/ecs/components/collider.hpp>
#include <rivet/ecs/components/transform.hpp>

namespace rivet::game::level {

namespace {

void spawn_collision(rivet::ecs::World& world, const LevelData& level) {
    using namespace rivet::ecs;
    using namespace rivet::ecs::components;

    const float cell_size = static_cast<float>(level.grid_size);
    for (int y = 0; y < level.height; ++y) {
        for (int x = 0; x < level.width; ++x) {
            if (level.collision[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] == 0) {
                continue;
            }

            const auto entity = world.create();
            world.registry().emplace<Transform>(
                entity,
                Transform{
                    .x = static_cast<float>(x) * cell_size,
                    .y = static_cast<float>(y) * cell_size,
                });
            world.registry().emplace<Collider>(
                entity,
                Collider{
                    .width = cell_size,
                    .height = cell_size,
                    .is_static = true,
                });
        }
    }
}

void spawn_object(rivet::ecs::World& world, const LevelObject& object, rivet::ecs::Entity& player_out) {
    using namespace rivet::ecs;
    using namespace rivet::ecs::components;

    const auto entity = world.create();
    world.registry().emplace<Transform>(entity, Transform{.x = object.x, .y = object.y});

    if (object.type == "player" || object.type == "patrol") {
        const float vel_x = object.type == "player" ? 0.0f : object.vel_x;
        const float vel_y = object.type == "player" ? 0.0f : object.vel_y;
        world.registry().emplace<Velocity>(entity, Velocity{.x = vel_x, .y = vel_y});
        world.registry().emplace<Collider>(
            entity,
            Collider{.width = object.width, .height = object.height, .is_static = false});
        if (object.type == "player") {
            player_out = entity;
        }
        return;
    }

    if (object.type == "block" || object.type == "wall") {
        world.registry().emplace<Collider>(
            entity,
            Collider{
                .width = object.width,
                .height = object.height,
                .is_static = true,
            });
        return;
    }

    throw std::runtime_error("Unsupported level object type: " + object.type);
}

} // namespace

LevelSpawnResult spawn_level(rivet::ecs::World& world, const LevelData& level) {
    LevelSpawnResult result;
    result.world_width = static_cast<float>(level.width * level.grid_size);
    result.world_height = static_cast<float>(level.height * level.grid_size);

    spawn_collision(world, level);

    for (const auto& object : level.objects) {
        spawn_object(world, object, result.player);
    }

    return result;
}

} // namespace rivet::game::level
