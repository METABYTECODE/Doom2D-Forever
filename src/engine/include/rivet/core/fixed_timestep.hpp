#pragma once

namespace rivet::core {

/// Accumulates frame time and exposes fixed-step consumption + render interpolation alpha.
class FixedTimestep {
public:
    explicit FixedTimestep(float step_seconds = 1.0f / 60.0f);

    void advance(float frame_dt);
    [[nodiscard]] bool consume_fixed_step();
    [[nodiscard]] float render_alpha() const;
    [[nodiscard]] float step_seconds() const { return step_seconds_; }

private:
    float step_seconds_;
    float accumulator_ = 0.0f;
};

} // namespace rivet::core
