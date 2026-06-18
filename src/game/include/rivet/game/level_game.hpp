#pragma once

#include <filesystem>

#include <rivet/api.hpp>
#include <rivet/game/level/level_data.hpp>

namespace rivet::game {

class LevelGame final : public rivet::core::Game {
public:
    explicit LevelGame(std::filesystem::path level_path);

    void on_attach(rivet::core::GameContext& context) override;
    void on_detach(rivet::core::GameContext& context) override;
    void on_update(rivet::core::GameContext& context, float delta_time) override;
    void on_fixed_update(rivet::core::GameContext& context, float fixed_delta_time) override;
    void on_render(rivet::core::GameContext& context, float interpolation_alpha) override;

private:
    std::filesystem::path level_path_;
    level::LevelData level_;
    rivet::physics::PhysicsWorld physics_;
    rivet::resources::TextureHandle player_texture_ = rivet::resources::kInvalidTexture;
};

} // namespace rivet::game
