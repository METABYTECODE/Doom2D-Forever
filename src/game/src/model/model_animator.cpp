#include <algorithm>
#include <cmath>

#include <rivet/game/model/model_anim_ids.hpp>
#include <rivet/game/model/model_animator.hpp>

namespace rivet::game::model {

void ModelAnimator::set_model(const ModelDef* model) {
    model_ = model;
    animation_ = nullptr;
    current_name_.clear();
    frame_index_ = 0;
    frame_time_ = 0.0f;
}

void ModelAnimator::set_animation(const std::string& name, const bool restart) {
    if (model_ == nullptr) {
        animation_ = nullptr;
        current_name_.clear();
        return;
    }

    const ModelAnimation* clip = model_->find_animation(name);
    if (clip == nullptr) {
        animation_ = nullptr;
        current_name_.clear();
        return;
    }

    if (!restart && current_name_ == name && animation_ == clip) {
        return;
    }

    current_name_ = name;
    animation_ = clip;
    frame_index_ = 0;
    frame_time_ = 0.0f;
}

void ModelAnimator::update(const float delta_time) {
    if (animation_ == nullptr || animation_->frame_ids.empty()) {
        return;
    }

    const int frame_count = static_cast<int>(animation_->frame_ids.size());
    const float frame_duration = static_cast<float>(animation_->frame_ms) / 1000.0f;
  frame_time_ += delta_time;

    while (frame_time_ >= frame_duration) {
        frame_time_ -= frame_duration;
        if (animation_->loop) {
            frame_index_ = (frame_index_ + 1) % frame_count;
        } else if (frame_index_ + 1 < frame_count) {
            frame_index_ += 1;
        }
    }
}

ModelDrawFrame ModelAnimator::current_frame() const {
    ModelDrawFrame frame;
    if (animation_ == nullptr || animation_->frame_ids.empty()) {
        return frame;
    }

    frame.texture = animation_->texture;
    frame.frame_width = animation_->frame_width;
    frame.frame_height = animation_->frame_height;
    frame.source = model_frame_source_rect(*animation_, frame_index_);
    return frame;
}

std::string select_player_locomotion_animation(
    const bool grounded,
    const float velocity_x,
    const float velocity_y) {
    using namespace anim;

    if (!grounded) {
        if (velocity_y < -32.0f) {
            return std::string(Jump);
        }
        if (velocity_y > 32.0f) {
            return std::string(Fall);
        }
        return std::string(Jump);
    }

    if (std::abs(velocity_x) > 8.0f) {
        return std::string(Walk);
    }

    return std::string(Idle);
}

} // namespace rivet::game::model
