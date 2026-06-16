#pragma once

#include <d2df/map/map_document.hpp>

#include <vector>

namespace d2df::sim {

/// Runtime lift zones (PANEL_LIFT* force fields).
class LiftSystem {
public:
    void reset(const std::vector<map::MapPanel>& panels);
    void apply_on_load(std::vector<map::MapPanel>& panels,
                         const std::vector<map::MapTrigger>& triggers);
    void toggle_group_for_zone(std::vector<map::MapPanel>& panels, std::int32_t zone_panel_index);
    void set_group_for_zone(std::vector<map::MapPanel>& panels, std::int32_t zone_panel_index,
                            bool up);

private:
    struct LiftGroup {
        std::vector<std::size_t> zone_panels;
    };

    void build_groups(const std::vector<map::MapPanel>& panels);
    static void set_group_direction(std::vector<map::MapPanel>& panels, const LiftGroup& group,
                                    bool up);

    std::vector<LiftGroup> groups_;
};

} // namespace d2df::sim
