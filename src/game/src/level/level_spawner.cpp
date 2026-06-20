#include <stdexcept>

#include <rivet/game/level/level_spawner.hpp>

#include <rivet/ecs/components/collider.hpp>
#include <rivet/ecs/components/transform.hpp>
#include <rivet/game/model/model_loader.hpp>

namespace rivet::game::level {

namespace {

[[nodiscard]] std::string resolve_model_id(const LevelObject& object) {
    if (!object.model.empty()) {
        return object.model;
    }
    if (object.type == "player") {
        return "player";
    }
    return {};
}

[[nodiscard]] model::ModelBody resolve_model_body(
    const LevelObject& object,
    const LevelSpawnContext& context) {
    const auto model_id = resolve_model_id(object);
    if (!model_id.empty() && context.pack != nullptr) {
        if (const auto model_path = context.pack->resolve_model(model_id)) {
            if (const auto body = model::load_model_body(*model_path)) {
                return *body;
            }
        }
    }

    if (object.type == "player" || !object.model.empty()) {
        return {
            .pivot = {.x = 16.0f, .y = 32.0f},
            .collider = {.x = 6.0f, .y = 8.0f, .width = 20.0f, .height = 24.0f},
        };
    }

    return {
        .pivot = {.x = 0.0f, .y = 0.0f},
        .collider = {
            .x = 0.0f,
            .y = 0.0f,
            .width = object.width,
            .height = object.height,
        },
    };
}

void spawn_object(
    rivet::ecs::World& world,
    const LevelObject& object,
    const LevelSpawnContext& context,
    rivet::ecs::Entity& player_out,
    std::vector<rivet::ecs::Entity>& patrols_out) {
    using namespace rivet::ecs;
    using namespace rivet::ecs::components;

    const auto entity = world.create();
    world.registry().emplace<Transform>(entity, Transform{.x = object.x, .y = object.y});

    if (object.type == "player" || object.type == "patrol") {
        const float vel_x = object.type == "player" ? 0.0f : object.vel_x;
        const float vel_y = object.type == "player" ? 0.0f : object.vel_y;
        const auto body = resolve_model_body(object, context);
        world.registry().emplace<Collider>(
            entity,
            Collider{
                .width = body.collider.width,
                .height = body.collider.height,
                .offset_x = body.collider.x - body.pivot.x,
                .offset_y = body.collider.y - body.pivot.y,
                .is_static = false,
            });
        world.registry().emplace<Velocity>(entity, Velocity{.x = vel_x, .y = vel_y});
        if (object.type == "player") {
            player_out = entity;
        } else {
            patrols_out.push_back(entity);
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

LevelSpawnResult spawn_level(
    rivet::ecs::World& world,
    const LevelData& level,
    const LevelSpawnContext& context) {
    LevelSpawnResult result;
    result.world_width = static_cast<float>(level.width);
    result.world_height = static_cast<float>(level.height);

    for (const auto& object : level.objects) {
        spawn_object(world, object, context, result.player, result.patrols);
    }

    return result;
}

} // namespace rivet::game::level
