#include <catch2/catch_test_macros.hpp>

#include <d2df/core/perf_stats.hpp>

using namespace d2df::core;

TEST_CASE("PerfStats rolling average tracks region samples", "[perf]") {
    auto& stats = PerfStats::instance();

    stats.begin_frame();
    stats.record(PerfRegion::SimFixed, 0.010);
    stats.end_frame(1.0 / 60.0);

    stats.begin_frame();
    stats.record(PerfRegion::SimFixed, 0.020);
    stats.end_frame(1.0 / 60.0);

    CHECK(stats.region_avg_ms(PerfRegion::SimFixed) == 15.0);
    CHECK(stats.region_max_ms(PerfRegion::SimFixed) == 20.0);
    CHECK(stats.fps() > 59.0);
}

TEST_CASE("ScopedPerfRegion records elapsed time", "[perf]") {
    auto& stats = PerfStats::instance();
    stats.begin_frame();

    ScopedPerfRegion scope(PerfRegion::MapRender);
    (void)scope;

    stats.end_frame(0.016);

    CHECK(stats.region_avg_ms(PerfRegion::MapRender) >= 0.0);
}
