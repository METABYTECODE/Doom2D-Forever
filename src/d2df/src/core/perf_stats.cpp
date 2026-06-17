#include <d2df/core/perf_stats.hpp>

#include <algorithm>
#include <cmath>

namespace d2df::core {

const char* perf_region_name(PerfRegion region) {
    switch (region) {
    case PerfRegion::Frame:
        return "frame";
    case PerfRegion::MapUpdate:
        return "map_update";
    case PerfRegion::SimFixed:
        return "sim_fixed";
    case PerfRegion::SimCollision:
        return "sim_collision";
    case PerfRegion::MapRender:
        return "map_render";
    case PerfRegion::MapRenderWorld:
        return "render_world";
    case PerfRegion::MapRenderHud:
        return "render_hud";
    case PerfRegion::DebugUi:
        return "debug_ui";
    case PerfRegion::Count:
        break;
    }
    return "unknown";
}

PerfStats& PerfStats::instance() {
    static PerfStats stats;
    return stats;
}

void PerfStats::begin_frame() {
    for (auto& accum : frame_accum_) {
        accum = 0.0;
    }
    sim_steps_last_frame_ = 0;
}

void PerfStats::end_frame(double wall_seconds) {
    if (wall_seconds > 0.0) {
        fps_ = 1.0 / wall_seconds;
        frame_ms_ = wall_seconds * 1000.0;
    }

    record(PerfRegion::Frame, wall_seconds);

    for (std::size_t i = 0; i < frame_accum_.size(); ++i) {
        auto& hist = history_[i];
        hist.samples[hist.index] = frame_accum_[i] * 1000.0;
        hist.index = (hist.index + 1) % kHistoryFrames;
        if (hist.count < kHistoryFrames) {
            ++hist.count;
        }
    }
}

void PerfStats::record(PerfRegion region, double seconds) {
    const auto index = static_cast<std::size_t>(region);
    if (index >= frame_accum_.size()) {
        return;
    }
    frame_accum_[index] += seconds;
}

double PerfStats::region_avg_ms(PerfRegion region) const {
    const auto index = static_cast<std::size_t>(region);
    if (index >= history_.size()) {
        return 0.0;
    }
    const auto& hist = history_[index];
    if (hist.count == 0) {
        return 0.0;
    }
    double sum = 0.0;
    for (std::size_t i = 0; i < hist.count; ++i) {
        sum += hist.samples[i];
    }
    return sum / static_cast<double>(hist.count);
}

double PerfStats::region_max_ms(PerfRegion region) const {
    const auto index = static_cast<std::size_t>(region);
    if (index >= history_.size()) {
        return 0.0;
    }
    const auto& hist = history_[index];
    if (hist.count == 0) {
        return 0.0;
    }
    double max_value = 0.0;
    for (std::size_t i = 0; i < hist.count; ++i) {
        max_value = std::max(max_value, hist.samples[i]);
    }
    return max_value;
}

ScopedPerfRegion::ScopedPerfRegion(PerfRegion region)
    : region_(region)
    , start_(std::chrono::steady_clock::now()) {}

ScopedPerfRegion::~ScopedPerfRegion() {
    const auto end = std::chrono::steady_clock::now();
    const double seconds =
        std::chrono::duration<double>(end - start_).count();
    PerfStats::instance().record(region_, seconds);
}

} // namespace d2df::core
