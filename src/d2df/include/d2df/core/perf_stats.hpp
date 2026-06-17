#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>

namespace d2df::core {

enum class PerfRegion : std::uint8_t {
    Frame = 0,
    MapUpdate = 1,
    SimFixed = 2,
    SimCollision = 3,
    MapRender = 4,
    MapRenderWorld = 5,
    MapRenderHud = 6,
    DebugUi = 7,
    Count
};

[[nodiscard]] const char* perf_region_name(PerfRegion region);

class PerfStats {
public:
    static constexpr std::size_t kHistoryFrames = 120;

    void begin_frame();
    void end_frame(double wall_seconds);

    void record(PerfRegion region, double seconds);

    [[nodiscard]] double fps() const { return fps_; }
    [[nodiscard]] double frame_ms() const { return frame_ms_; }
    [[nodiscard]] double region_avg_ms(PerfRegion region) const;
    [[nodiscard]] double region_max_ms(PerfRegion region) const;
    [[nodiscard]] std::uint32_t sim_steps_last_frame() const { return sim_steps_last_frame_; }

    void set_sim_steps(std::uint32_t steps) { sim_steps_last_frame_ = steps; }

    [[nodiscard]] static PerfStats& instance();

private:
    struct RegionHistory {
        std::array<double, kHistoryFrames> samples{};
        std::size_t count = 0;
        std::size_t index = 0;
    };

    std::array<double, static_cast<std::size_t>(PerfRegion::Count)> frame_accum_{};
    std::array<RegionHistory, static_cast<std::size_t>(PerfRegion::Count)> history_{};
    double fps_ = 0.0;
    double frame_ms_ = 0.0;
    std::uint32_t sim_steps_last_frame_ = 0;
};

class ScopedPerfRegion {
public:
    explicit ScopedPerfRegion(PerfRegion region);
    ~ScopedPerfRegion();

    ScopedPerfRegion(const ScopedPerfRegion&) = delete;
    ScopedPerfRegion& operator=(const ScopedPerfRegion&) = delete;

private:
    PerfRegion region_;
    std::chrono::steady_clock::time_point start_;
};

} // namespace d2df::core
