#include <rivet/game/model/model_types.hpp>

namespace rivet::game::model {

const ModelAnimation* ModelDef::find_animation(const std::string& animation_name) const {
    const auto it = animations.find(animation_name);
    if (it == animations.end()) {
        return nullptr;
    }
    return &it->second;
}

rivet::render::Rect model_frame_source_rect(const ModelAnimation& animation, const int frame_index) {
    const int safe_index = frame_index >= 0 && frame_index < static_cast<int>(animation.frame_ids.size())
                               ? frame_index
                               : 0;
    const int frame_id = animation.frame_ids[static_cast<std::size_t>(safe_index)];
    const int col = frame_id % animation.columns;
    const int row = frame_id / animation.columns;
    return {
        .x = static_cast<float>(col * animation.frame_width),
        .y = static_cast<float>(row * animation.frame_height),
        .width = static_cast<float>(animation.frame_width),
        .height = static_cast<float>(animation.frame_height),
    };
}

rivet::render::Rect model_sprite_dest_rect(
    const float pivot_x,
    const float pivot_y,
    const ModelPivot& pivot,
    const int frame_width,
    const int frame_height) {
    return {
        .x = pivot_x - pivot.x,
        .y = pivot_y - pivot.y,
        .width = static_cast<float>(frame_width),
        .height = static_cast<float>(frame_height),
    };
}

} // namespace rivet::game::model
