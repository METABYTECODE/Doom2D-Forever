#pragma once

#include <d2df/map/map_document.hpp>
#include <d2df/sim/lift_system.hpp>
#include <d2df/sim/map_collision.hpp>
#include <d2df/sim/player_state.hpp>

namespace d2df {

class EventBus;

} // namespace d2df

namespace d2df::sim {

/// Map triggers + runtime panel state (doors, lifts, teleports).
class TriggerSystem {
public:
    void reset(const map::MapDocument& map);
    void update(PlayerState& player, bool use_pressed, EventBus* events = nullptr);

    void press_shot_line(float x1, float y1, float x2, float y2, PlayerState& player,
                         EventBus* events);
    void press_shot_rect(float x, float y, float width, float height, PlayerState& player,
                         EventBus* events);
    void press_shot_circle(float cx, float cy, float radius, PlayerState& player,
                           EventBus* events);

    [[nodiscard]] bool consume_exit_request();
    [[nodiscard]] const std::vector<map::MapPanel>& panels() const { return panels_; }
    [[nodiscard]] map::MapDocument map_view(const map::MapDocument& base) const;

private:
    void init_panel_defaults();
    void build_door_groups();
    void apply_on_load_triggers();
    void activate_trigger(std::size_t trigger_index, PlayerState& player, EventBus* events);
    void set_door_group_open(std::int32_t panel_index, bool open);
    void close_trap_group(std::int32_t panel_index, PlayerState& player, EventBus* events);
    void tick_door_timers();
    void resolve_missing_expander_zones();
    void tick_expanders(PlayerState& player, EventBus* events);
    void queue_expander(std::size_t trigger_index);
    void apply_expander_effects(std::size_t trigger_index, PlayerState& player, EventBus* events);
    void mark_trigger_used(std::size_t trigger_index);
    [[nodiscard]] bool any_monsters_in_trigger(const map::MapTrigger& trigger) const;

    [[nodiscard]] bool player_overlaps(const map::MapTrigger& trigger,
                                       const PlayerState& player) const;
    [[nodiscard]] bool player_overlaps_panel(const map::MapPanel& panel,
                                             const PlayerState& player) const;
    [[nodiscard]] bool trigger_affects(const map::MapTrigger& source,
                                       const map::MapTrigger& target) const;

    map::MapDocument source_;
    std::vector<map::MapPanel> panels_;
    std::vector<map::MapTrigger> triggers_;
    std::vector<std::vector<std::size_t>> door_groups_;
    struct ExpanderRuntime {
        int press_count = 0;
        int press_time = -1;
    };

    std::vector<int> trigger_timeout_;
    std::vector<ExpanderRuntime> expander_;
    std::vector<bool> nomonster_boot_done_;
    std::vector<std::pair<std::int32_t, int>> trap_timers_;
    std::vector<std::pair<std::int32_t, int>> door5_timers_;
    bool exit_requested_ = false;
    LiftSystem lift_;
};

} // namespace d2df::sim
