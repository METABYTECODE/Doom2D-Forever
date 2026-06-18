#pragma once

#include <cstdint>

namespace rivet::core {

/// High-level frame clock: delta time in seconds since the last tick.
class Clock {
public:
    void tick(float frame_dt_seconds);
    [[nodiscard]] float delta_time() const { return delta_time_; }
    [[nodiscard]] float total_time() const { return total_time_; }
    [[nodiscard]] std::uint64_t frame_index() const { return frame_index_; }

private:
    float delta_time_ = 0.0f;
    float total_time_ = 0.0f;
    std::uint64_t frame_index_ = 0;
};

} // namespace rivet::core
