#pragma once

#include <rivet/ecs/world.hpp>
#include <rivet/scene/scene.hpp>

namespace rivet::game {

class SandboxScene final : public rivet::scene::Scene {
public:
    SandboxScene();

    [[nodiscard]] rivet::ecs::Entity player_entity() const { return player_; }

protected:
    void on_enter() override;

private:
    rivet::ecs::Entity player_ = rivet::ecs::kNullEntity;
};

} // namespace rivet::game
