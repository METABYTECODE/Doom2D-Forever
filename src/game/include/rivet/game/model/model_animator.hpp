#pragma once

#include <string>

#include <rivet/game/model/model_types.hpp>

namespace rivet::game::model {

struct ModelDrawFrame {
    rivet::resources::TextureHandle texture = rivet::resources::kInvalidTexture;
    rivet::render::Rect source{};
    int frame_width = 0;
    int frame_height = 0;
};

/// Advances clip timing and resolves atlas UVs for the current frame.
class ModelAnimator {
public:
    void set_model(const ModelDef* model);
    void set_animation(const std::string& name, bool restart = true);
    [[nodiscard]] const std::string& current_animation() const { return current_name_; }

    void update(float delta_time);

    [[nodiscard]] ModelDrawFrame current_frame() const;
    [[nodiscard]] bool has_clip() const { return animation_ != nullptr; }

private:
    const ModelDef* model_ = nullptr;
    const ModelAnimation* animation_ = nullptr;
    std::string current_name_;
    int frame_index_ = 0;
    float frame_time_ = 0.0f;
};

[[nodiscard]] std::string select_player_locomotion_animation(
    bool grounded,
    float velocity_x,
    float velocity_y);

} // namespace rivet::game::model
