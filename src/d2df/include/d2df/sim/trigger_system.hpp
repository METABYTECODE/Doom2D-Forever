#pragma once

#include <d2df/map/map_document.hpp>
#include <d2df/sim/lift_system.hpp>
#include <d2df/sim/map_collision.hpp>
#include <d2df/sim/player_state.hpp>

namespace d2df::sim {

/// Map triggers + runtime panel state (doors, lifts, teleports).
class TriggerSystem {
public:
    void reset(const map::MapDocument& map);
    void update(PlayerState& player, bool use_pressed);

    [[nodiscard]] const std::vector<map::MapPanel>& panels() const { return panels_; }
    [[nodiscard]] map::MapDocument map_view(const map::MapDocument& base) const;

private:
    void init_panel_defaults();
    void build_door_groups();
    void apply_on_load_triggers();
    void activate_trigger(std::size_t trigger_index, PlayerState& player);
    void set_door_group_open(std::int32_t panel_index, bool open);
    void close_trap_group(std::int32_t panel_index);
    void tick_trap_timers();

    [[nodiscard]] bool player_overlaps(const map::MapTrigger& trigger, const PlayerState& player) const;
    [[nodiscard]] bool trigger_affects(const map::MapTrigger& source,
                                       const map::MapTrigger& target) const;

    map::MapDocument source_;
    std::vector<map::MapPanel> panels_;
    std::vector<map::MapTrigger> triggers_;
    std::vector<std::vector<std::size_t>> door_groups_;
    std::vector<bool> collide_was_inside_;
    std::vector<std::pair<std::int32_t, int>> trap_timers_;
    LiftSystem lift_;
};

} // namespace d2df::sim
