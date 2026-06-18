#include <rivet/game/bounce_scene.hpp>

#include <rivet/ecs/components/collider.hpp>
#include <rivet/ecs/components/transform.hpp>

namespace rivet::game {

namespace {

constexpr float kPlayerSize = 40.0f;

} // namespace

BounceScene::BounceScene()
    : Scene("bounce") {}

void BounceScene::on_enter() {
    using namespace rivet::ecs;
    using namespace rivet::ecs::components;

    player_ = world().create();
    world().registry().emplace<Transform>(player_, Transform{.x = 360.0f, .y = 260.0f});
    world().registry().emplace<Velocity>(player_, Velocity{.x = 0.0f, .y = 0.0f});
    world().registry().emplace<Collider>(player_, Collider{.width = kPlayerSize, .height = kPlayerSize});
}

} // namespace rivet::game
