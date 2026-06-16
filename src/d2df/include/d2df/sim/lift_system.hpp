#pragma once

#include <d2df/map/map_document.hpp>

namespace d2df::sim {

/// Runtime lift zones (PANEL_LIFT* force fields).
class LiftSystem {
public:
    void reset(const std::vector<map::MapPanel>& panels);
    void apply_on_load(const std::vector<map::MapTrigger>& triggers);
    void toggle_group_for_zone(std::int32_t zone_panel_index);
    void set_group_for_zone(std::int32_t zone_panel_index, bool up);

    [[nodiscard]] const std::vector<map::MapPanel>& panels() const { return panels_; }

private:
    struct LiftGroup {
        std::vector<std::size_t> zone_panels;
    };

    void build_groups();
    void set_group_direction(LiftGroup& group, bool up);

    std::vector<map::MapPanel> panels_;
    std::vector<LiftGroup> groups_;
};

} // namespace d2df::sim
