#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <rivet/render/rect.hpp>
#include <rivet/resources/texture.hpp>

namespace rivet::game::model {

/// Entity attachment point in reference frame space (top-left origin).
struct ModelPivot {
    float x = 32.0f;
    float y = 64.0f;
};

/// Physics hull in reference frame space (same coords as sprite frame).
struct ModelCollider {
    float x = 15.0f;
    float y = 12.0f;
    float width = 34.0f;
    float height = 52.0f;
};

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
    ModelPivot pivot;
    ModelCollider collider;
    std::unordered_map<std::string, ModelAnimation> animations;

    [[nodiscard]] const ModelAnimation* find_animation(const std::string& name) const;
    [[nodiscard]] float collider_offset_x() const { return collider.x - pivot.x; }
    [[nodiscard]] float collider_offset_y() const { return collider.y - pivot.y; }
};

[[nodiscard]] rivet::render::Rect model_frame_source_rect(const ModelAnimation& animation, int frame_index);

[[nodiscard]] rivet::render::Rect model_sprite_dest_rect(
    float pivot_x,
    float pivot_y,
    const ModelPivot& pivot,
    int frame_width,
    int frame_height);

} // namespace rivet::game::model
