#include <rivet/core/fixed_timestep.hpp>

#include <algorithm>

namespace rivet::core {

FixedTimestep::FixedTimestep(float step_seconds)
    : step_seconds_(step_seconds) {}

void FixedTimestep::advance(float frame_dt) {
    accumulator_ += std::max(frame_dt, 0.0f);
}

bool FixedTimestep::consume_fixed_step() {
    if (accumulator_ < step_seconds_) {
        return false;
    }
    accumulator_ -= step_seconds_;
    return true;
}

float FixedTimestep::render_alpha() const {
    if (step_seconds_ <= 0.0f) {
        return 0.0f;
    }
    return std::clamp(accumulator_ / step_seconds_, 0.0f, 1.0f);
}

} // namespace rivet::core
