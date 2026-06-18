#include <rivet/game/sandbox_scene.hpp>

#include <rivet/ecs/components/collider.hpp>
#include <rivet/ecs/components/transform.hpp>

namespace rivet::game {

namespace {

constexpr float kSpriteSize = 48.0f;

} // namespace

SandboxScene::SandboxScene()
    : Scene("sandbox") {}

void SandboxScene::on_enter() {
    using namespace rivet::ecs;
    using namespace rivet::ecs::components;

    player_ = world().create();
    world().registry().emplace<Transform>(player_, Transform{.x = 200.0f, .y = 200.0f});
    world().registry().emplace<Velocity>(player_, Velocity{.x = 220.0f, .y = 160.0f});
    world().registry().emplace<Collider>(player_, Collider{.width = kSpriteSize, .height = kSpriteSize});

    const auto wall = world().create();
    world().registry().emplace<Transform>(wall, Transform{.x = 520.0f, .y = 360.0f});
    world().registry().emplace<Collider>(
        wall,
        Collider{.width = 220.0f, .height = 64.0f, .is_static = true});

    const auto pillar = world().create();
    world().registry().emplace<Transform>(pillar, Transform{.x = 900.0f, .y = 700.0f});
    world().registry().emplace<Collider>(
        pillar,
        Collider{.width = 96.0f, .height = 96.0f, .is_static = true});
}

} // namespace rivet::game
