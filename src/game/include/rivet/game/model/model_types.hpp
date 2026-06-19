#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <rivet/render/rect.hpp>
#include <rivet/resources/texture.hpp>

namespace rivet::game::model {

struct ModelAnimation {
    std::string atlas;
    int frame_width = 64;
    int frame_height = 64;
    int columns = 1;
    std::vector<int> frame_ids;
    int frame_ms = 120;
    bool loop = true;
    rivet::resources::TextureHandle texture = rivet::resources::kInvalidTexture;
};

struct ModelDef {
    std::string id;
    std::string name;
    std::string resource_pack;
    std::unordered_map<std::string, ModelAnimation> animations;

    [[nodiscard]] const ModelAnimation* find_animation(const std::string& name) const;
};

[[nodiscard]] rivet::render::Rect model_frame_source_rect(const ModelAnimation& animation, int frame_index);

/// Bottom-center of collider box aligns with sprite feet; sprite drawn above origin.
[[nodiscard]] rivet::render::Rect model_sprite_dest_rect(
    float origin_x,
    float origin_y,
    float collider_width,
    float collider_height,
    int frame_width,
    int frame_height);

} // namespace rivet::game::model
