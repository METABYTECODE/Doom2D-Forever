#pragma once

#include <filesystem>
#include <memory>
#include <optional>

#include <rivet/api.hpp>
#include <rivet/game/level_scene.hpp>
#include <rivet/game/level/level_data.hpp>
#include <rivet/game/tileset/tileset_catalog.hpp>

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
    void sync_player_motion(const LevelScene& scene);
    void update_camera(
        rivet::core::GameContext& context,
        float player_center_x,
        float player_center_y) const;
    void draw_level_tiles(rivet::render::IRenderer& renderer, const level::LevelData& data, float animation_time);

    std::filesystem::path level_path_;
    level::LevelData level_;
    rivet::physics::PhysicsWorld physics_;
    rivet::resources::TextureHandle player_texture_ = rivet::resources::kInvalidTexture;
    std::unique_ptr<tileset::TilesetCatalog> tilesets_;
    std::filesystem::path assets_root_;
    float animation_time_ = 0.0f;

    struct PlayerMotion {
        float prev_x = 0.0f;
        float prev_y = 0.0f;
        float curr_x = 0.0f;
        float curr_y = 0.0f;
    } player_motion_;

    bool player_grounded_ = false;
    float jump_buffer_time_ = 0.0f;
    float coyote_time_ = 0.0f;
};

} // namespace rivet::game
