#include <catch2/catch_test_macros.hpp>

#include <d2df/core/perf_stats.hpp>
#include <d2df/map/map_document.hpp>
#include <d2df/map/panel_types.hpp>
#include <d2df/render/map_render_index.hpp>

using namespace d2df::core;

TEST_CASE("MapRenderIndex buckets panels by layer", "[perf][render]") {
    d2df::map::MapDocument document;
    document.panels.resize(3);

    document.panels[0].type = d2df::map::PANEL_WALL;
    document.panels[1].type = d2df::map::PANEL_WATER;
    document.panels[2].type = d2df::map::PANEL_WALL | d2df::map::PANEL_FORE;
    document.panels[2].flags = d2df::map::PANEL_FLAG_HIDE;

    d2df::render::MapRenderIndex index;
    index.build(document);

    CHECK(index.layer_panels(2).size() == 1);
    CHECK(index.layer_panels(2).front() == 0);
    CHECK(index.layer_panels(7).size() == 1);
    CHECK(index.layer_panels(7).front() == 1);
    CHECK(index.layer_panels(8).empty());
}

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
