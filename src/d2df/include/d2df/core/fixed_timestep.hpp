#pragma once

namespace d2df {

/// Fixed simulation rate matching legacy Doom2D (36 UPS).
class FixedTimestep {
public:
    explicit FixedTimestep(float step_seconds = 1.0f / 36.0f);

    void advance(float frame_dt);
    [[nodiscard]] bool consume_fixed_step();
    [[nodiscard]] float render_alpha() const;
    [[nodiscard]] float step_seconds() const { return step_seconds_; }

private:
    float step_seconds_;
    float accumulator_ = 0.0f;
};

} // namespace d2df
