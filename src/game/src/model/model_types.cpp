#include <algorithm>

#include <rivet/game/model/model_types.hpp>

namespace rivet::game::model {

const ModelAnimation* ModelDef::find_animation(const std::string& animation_name) const {
    if (const auto it = animations.find(animation_name); it != animations.end()) {
        return &it->second;
    }
    if (const auto idle = animations.find("IDLE"); idle != animations.end()) {
        return &idle->second;
    }
    if (!animations.empty()) {
        return &animations.begin()->second;
    }
    return nullptr;
}

rivet::render::Rect model_frame_source_rect(const ModelAnimation& animation, const int frame_index) {
    if (frame_index < 0 || frame_index >= static_cast<int>(animation.frame_ids.size())) {
        return {};
    }

    const int tile_id = animation.frame_ids[static_cast<std::size_t>(frame_index)];
    const int columns = std::max(1, animation.columns);
    const int col = tile_id % columns;
    const int row = tile_id / columns;
    return {
        .x = static_cast<float>(col * animation.frame_width),
        .y = static_cast<float>(row * animation.frame_height),
        .width = static_cast<float>(animation.frame_width),
        .height = static_cast<float>(animation.frame_height),
    };
}

rivet::render::Rect model_sprite_dest_rect(
    const float origin_x,
    const float origin_y,
    const float collider_width,
    const float collider_height,
    const int frame_width,
    const int frame_height) {
    return {
        .x = origin_x + (collider_width - static_cast<float>(frame_width)) * 0.5f,
        .y = origin_y + collider_height - static_cast<float>(frame_height),
        .width = static_cast<float>(frame_width),
        .height = static_cast<float>(frame_height),
    };
}

} // namespace rivet::game::model
