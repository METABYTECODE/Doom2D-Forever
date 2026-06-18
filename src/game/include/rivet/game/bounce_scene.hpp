#pragma once

#include <rivet/api.hpp>

namespace rivet::game {

class BounceScene final : public rivet::scene::Scene {
public:
    BounceScene();

    [[nodiscard]] rivet::ecs::Entity player_entity() const { return player_; }

protected:
    void on_enter() override;

private:
    rivet::ecs::Entity player_ = rivet::ecs::kNullEntity;
};

} // namespace rivet::game
