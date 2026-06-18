#include <rivet/core/clock.hpp>

#include <algorithm>

namespace rivet::core {

void Clock::tick(float frame_dt_seconds) {
    delta_time_ = std::max(frame_dt_seconds, 0.0f);
    total_time_ += delta_time_;
    ++frame_index_;
}

} // namespace rivet::core
