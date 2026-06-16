#include <d2df/sim/trigger_system.hpp>

#include <algorithm>

#include <d2df/core/event_bus.hpp>
#include <d2df/core/game_events.hpp>
#include <d2df/map/panel_types.hpp>
#include <d2df/map/trigger_types.hpp>
#include <d2df/sim/combat_common.hpp>
#include <spdlog/spdlog.h>

namespace d2df::sim {
namespace {

constexpr std::uint16_t kDoorMask = map::PANEL_CLOSEDOOR | map::PANEL_OPENDOOR;
constexpr int kTrapReopenTicks = 40;
constexpr int kDoor5CloseTicks = 180;

void publish_trap_damage(EventBus* events, int health_before, const PlayerState& player) {
    if (events == nullptr) {
        return;
    }
    const int dealt = health_before - player.health();
    if (dealt <= 0) {
        return;
    }
    events->publish(events::PlayerDamaged{dealt, events::DamageSource::Trap, player.health()});
    if (!player.alive()) {
        events->publish(events::PlayerDied{events::DamageSource::Trap});
    }
}

bool is_door_panel(const map::MapPanel& panel) {
    return (panel.type & kDoorMask) != 0;
}

bool panels_touch(const map::MapPanel& a, const map::MapPanel& b) {
    const float ax0 = static_cast<float>(a.position.x) - 1.0f;
    const float ay0 = static_cast<float>(a.position.y) - 1.0f;
    const float ax1 = static_cast<float>(a.position.x + a.size.width) + 1.0f;
    const float ay1 = static_cast<float>(a.position.y + a.size.height) + 1.0f;

    const float bx0 = static_cast<float>(b.position.x);
    const float by0 = static_cast<float>(b.position.y);
    const float bx1 = static_cast<float>(b.position.x + b.size.width);
    const float by1 = static_cast<float>(b.position.y + b.size.height);

    return ax0 < bx1 && ax1 > bx0 && ay0 < by1 && ay1 > by0;
}

bool has_flag(map::ActivateType flags, map::ActivateType flag) {
    return (static_cast<std::uint8_t>(flags) & static_cast<std::uint8_t>(flag)) != 0;
}

bool is_expander_type(map::TriggerType type) {
    switch (type) {
    case map::TriggerType::Press:
    case map::TriggerType::On:
    case map::TriggerType::Off:
    case map::TriggerType::OnOff:
        return true;
    default:
        return false;
    }
}

} // namespace

void TriggerSystem::resolve_missing_expander_zones() {
    constexpr int kSearchMargin = 128;

    for (auto& source : triggers_) {
        if (!is_expander_type(source.type)) {
            continue;
        }
        if (source.press_size.width > 0 && source.press_size.height > 0) {
            continue;
        }

        const float press_x0 = static_cast<float>(source.position.x);
        const float press_y0 = static_cast<float>(source.position.y);
        const float press_x1 = press_x0 + static_cast<float>(source.size.width);
        const float press_y1 = press_y0 + static_cast<float>(source.size.height);
        const float search_x0 = press_x0 - static_cast<float>(kSearchMargin);
        const float search_y0 = press_y0 - static_cast<float>(kSearchMargin);
        const float search_x1 = press_x1 + static_cast<float>(kSearchMargin);
        const float search_y1 = press_y1 + static_cast<float>(kSearchMargin);

        bool found = false;
        int min_x = 0;
        int min_y = 0;
        int max_x = 0;
        int max_y = 0;

        for (const auto& target : triggers_) {
            if (&target == &source || !target.enabled ||
                target.activate != map::ActivateType::None) {
                continue;
            }

            const float target_x0 = static_cast<float>(target.position.x);
            const float target_y0 = static_cast<float>(target.position.y);
            const float target_x1 = target_x0 + static_cast<float>(target.size.width);
            const float target_y1 = target_y0 + static_cast<float>(target.size.height);

            if (target_x0 >= search_x1 || target_x1 <= search_x0 || target_y0 >= search_y1 ||
                target_y1 <= search_y0) {
                continue;
            }

            if (!found) {
                min_x = target.position.x;
                min_y = target.position.y;
                max_x = target.position.x + target.size.width;
                max_y = target.position.y + target.size.height;
                found = true;
            } else {
                min_x = std::min(min_x, target.position.x);
                min_y = std::min(min_y, target.position.y);
                max_x = std::max(max_x, target.position.x + target.size.width);
                max_y = std::max(max_y, target.position.y + target.size.height);
            }
        }

        if (!found) {
            continue;
        }

        source.press_position = {min_x, min_y};
        source.press_size = {max_x - min_x, max_y - min_y};
        spdlog::info(
            "TriggerSystem: inferred expander zone at ({},{}) -> ({},{}) {}x{}",
            source.position.x, source.position.y, min_x, min_y, max_x - min_x, max_y - min_y);
    }
}

void TriggerSystem::reset(const map::MapDocument& map) {
    source_ = map;
    panels_ = map.panels;
    triggers_ = map.triggers;
    door_groups_.clear();
    trap_timers_.clear();
    door5_timers_.clear();
    exit_requested_ = false;
    trigger_timeout_.assign(triggers_.size(), 0);
    expander_.assign(triggers_.size(), {});
    nomonster_boot_done_.assign(triggers_.size(), false);

    resolve_missing_expander_zones();
    init_panel_defaults();
    build_door_groups();
    lift_.reset(panels_);
    lift_.apply_on_load(panels_, triggers_);
    apply_on_load_triggers();

    PlayerState dummy;
    dummy.snap_to(0.0f, 0.0f);
    tick_expanders(dummy, nullptr);

    spdlog::info("TriggerSystem: {} triggers, {} door groups", triggers_.size(),
                 door_groups_.size());
}

void TriggerSystem::init_panel_defaults() {
    for (auto& panel : panels_) {
        if ((panel.type & map::PANEL_OPENDOOR) != 0) {
            panel.enabled = false;
        } else if ((panel.type & map::PANEL_CLOSEDOOR) != 0) {
            panel.enabled = true;
        } else {
            panel.enabled = true;
        }
    }
}

void TriggerSystem::build_door_groups() {
    std::vector<std::size_t> door_indices;
    for (std::size_t i = 0; i < panels_.size(); ++i) {
        if (!is_door_panel(panels_[i])) {
            continue;
        }
        door_indices.push_back(i);
    }

    std::vector<bool> assigned(door_indices.size(), false);
    for (std::size_t seed = 0; seed < door_indices.size(); ++seed) {
        if (assigned[seed]) {
            continue;
        }

        std::vector<std::size_t> group;
        group.push_back(door_indices[seed]);
        assigned[seed] = true;

        bool expanded = true;
        while (expanded) {
            expanded = false;
            for (std::size_t candidate = 0; candidate < door_indices.size(); ++candidate) {
                if (assigned[candidate]) {
                    continue;
                }

                const auto& candidate_panel = panels_[door_indices[candidate]];
                for (const auto member_index : group) {
                    if (panels_touch(candidate_panel, panels_[member_index])) {
                        group.push_back(door_indices[candidate]);
                        assigned[candidate] = true;
                        expanded = true;
                        break;
                    }
                }
            }
        }

        door_groups_.push_back(std::move(group));
    }
}

void TriggerSystem::set_door_group_open(std::int32_t panel_index, bool open) {
    if (panel_index < 0) {
        return;
    }

    for (const auto& group : door_groups_) {
        const auto found =
            std::find_if(group.begin(), group.end(), [&](std::size_t idx) {
                return static_cast<std::int32_t>(idx) == panel_index;
            });
        if (found == group.end()) {
            continue;
        }

        for (const auto idx : group) {
            panels_[idx].enabled = !open;
        }
        return;
    }

    if (static_cast<std::size_t>(panel_index) < panels_.size()) {
        panels_[panel_index].enabled = !open;
    }
}

void TriggerSystem::close_trap_group(std::int32_t panel_index, PlayerState& player,
                                     EventBus* events) {
    if (panel_index < 0) {
        return;
    }

    auto close_indices = [&](const std::vector<std::size_t>& group) {
        for (const auto idx : group) {
            if (!panels_[idx].enabled && player_overlaps_panel(panels_[idx], player)) {
                const int health_before = player.health();
                player.apply_damage(PlayerState::kTrapDamage);
                publish_trap_damage(events, health_before, player);
            }
            if (!panels_[idx].enabled) {
                panels_[idx].enabled = true;
            }
        }
    };

    for (const auto& group : door_groups_) {
        const auto found =
            std::find_if(group.begin(), group.end(), [&](std::size_t idx) {
                return static_cast<std::int32_t>(idx) == panel_index;
            });
        if (found == group.end()) {
            continue;
        }

        close_indices(group);
        return;
    }

    if (static_cast<std::size_t>(panel_index) < panels_.size() &&
        !panels_[panel_index].enabled) {
        if (player_overlaps_panel(panels_[panel_index], player)) {
            const int health_before = player.health();
            player.apply_damage(PlayerState::kTrapDamage);
            publish_trap_damage(events, health_before, player);
        }
        panels_[panel_index].enabled = true;
    }
}

void TriggerSystem::tick_door_timers() {
    for (auto& timer : trap_timers_) {
        if (timer.second <= 0) {
            continue;
        }
        --timer.second;
        if (timer.second == 0) {
            set_door_group_open(timer.first, true);
        }
    }

    for (auto& timer : door5_timers_) {
        if (timer.second <= 0) {
            continue;
        }
        --timer.second;
        if (timer.second == 0) {
            set_door_group_open(timer.first, false);
            timer.second = -1;
        }
    }
}

bool TriggerSystem::consume_exit_request() {
    const bool requested = exit_requested_;
    exit_requested_ = false;
    return requested;
}

bool TriggerSystem::player_overlaps_panel(const map::MapPanel& panel,
                                          const PlayerState& player) const {
    return rects_overlap(player.x, player.y, player.width, player.height,
                         static_cast<float>(panel.position.x),
                         static_cast<float>(panel.position.y),
                         static_cast<float>(panel.size.width),
                         static_cast<float>(panel.size.height));
}

bool TriggerSystem::player_overlaps(const map::MapTrigger& trigger,
                                    const PlayerState& player) const {
    const float tx = static_cast<float>(trigger.position.x);
    const float ty = static_cast<float>(trigger.position.y);
    const float tw = static_cast<float>(trigger.size.width);
    const float th = static_cast<float>(trigger.size.height);

    if (rects_overlap(player.x, player.y, player.width, player.height, tx, ty, tw, th)) {
        return true;
    }
    if (rects_overlap(player.prev_x, player.prev_y, player.width, player.height, tx, ty, tw, th)) {
        return true;
    }

    const float swept_x = std::min(player.x, player.prev_x);
    const float swept_y = std::min(player.y, player.prev_y);
    const float swept_w =
        std::max(player.x + player.width, player.prev_x + player.width) - swept_x;
    const float swept_h =
        std::max(player.y + player.height, player.prev_y + player.height) - swept_y;
    return rects_overlap(swept_x, swept_y, swept_w, swept_h, tx, ty, tw, th);
}

bool TriggerSystem::trigger_affects(const map::MapTrigger& source,
                                    const map::MapTrigger& target) const {
    const float ax = static_cast<float>(source.press_position.x);
    const float ay = static_cast<float>(source.press_position.y);
    const float aw = static_cast<float>(source.press_size.width);
    const float ah = static_cast<float>(source.press_size.height);
    if (aw <= 0 || ah <= 0) {
        return false;
    }

    return rects_overlap(ax, ay, aw, ah, static_cast<float>(target.position.x),
                         static_cast<float>(target.position.y),
                         static_cast<float>(target.size.width),
                         static_cast<float>(target.size.height));
}

bool TriggerSystem::player_has_trigger_keys(const PlayerState& player, std::uint8_t required_keys) {
    if (required_keys == 0) {
        return true;
    }
    return (player.key_mask() & required_keys) == required_keys;
}

bool TriggerSystem::any_monsters_in_trigger(
    const map::MapTrigger& trigger, const std::vector<ShootableTarget>* monsters) const {
    if (monsters == nullptr) {
        return false;
    }

    const float tx = static_cast<float>(trigger.position.x);
    const float ty = static_cast<float>(trigger.position.y);
    const float tw = static_cast<float>(trigger.size.width);
    const float th = static_cast<float>(trigger.size.height);

    for (const auto& monster : *monsters) {
        if (!monster.alive() || !monster.is_monster()) {
            continue;
        }
        if (rects_overlap(tx, ty, tw, th, monster.x, monster.y, monster.width, monster.height)) {
            return true;
        }
    }
    return false;
}

bool TriggerSystem::try_teleport_player(PlayerState& player, float tx, float ty,
                                       EventBus* events) {
    if (active_collision_ != nullptr &&
        active_collision_->overlaps_solid(tx, ty, PlayerState::width, PlayerState::height)) {
        if (events != nullptr) {
            events->publish(events::PlayerTeleported{true});
        }
        return false;
    }
    player.snap_to(tx, ty);
    if (events != nullptr) {
        events->publish(events::PlayerTeleported{false});
    }
    return true;
}

void TriggerSystem::try_activate_trigger(std::size_t trigger_index, PlayerState& player,
                                           EventBus* events,
                                           const std::vector<ShootableTarget>* /*monsters*/) {
    const auto& trigger = triggers_[trigger_index];
    if (!player_has_trigger_keys(player, trigger.keys)) {
        return;
    }

    activate_trigger(trigger_index, player, events);
}

void TriggerSystem::queue_expander(std::size_t trigger_index) {
    auto& runtime = expander_[trigger_index];
    ++runtime.press_count;
    if (runtime.press_time < 0) {
        runtime.press_time = triggers_[trigger_index].press_wait;
    }
}

void TriggerSystem::apply_expander_effects(std::size_t trigger_index, PlayerState& player,
                                           EventBus* events) {
    const auto& source = triggers_[trigger_index];
    std::vector<std::size_t> affected;

    for (std::size_t i = 0; i < triggers_.size(); ++i) {
        if (!triggers_[i].enabled) {
            continue;
        }
        if (i == trigger_index && source.press_wait <= 0) {
            continue;
        }
        if (!trigger_affects(source, triggers_[i])) {
            continue;
        }
        affected.push_back(i);
    }

    if (source.type == map::TriggerType::Press && source.ext_random && !affected.empty()) {
        const std::size_t pick = affected.front();
        activate_trigger(pick, player, events);
        return;
    }

    for (const std::size_t target_index : affected) {
        switch (source.type) {
        case map::TriggerType::Press:
            activate_trigger(target_index, player, events);
            break;
        case map::TriggerType::On:
            triggers_[target_index].enabled = true;
            break;
        case map::TriggerType::Off:
            triggers_[target_index].enabled = false;
            trigger_timeout_[target_index] = 0;
            break;
        case map::TriggerType::OnOff:
            triggers_[target_index].enabled = !triggers_[target_index].enabled;
            if (!triggers_[target_index].enabled) {
                trigger_timeout_[target_index] = 0;
            }
            break;
        default:
            break;
        }
    }
}

void TriggerSystem::tick_expanders(PlayerState& player, EventBus* events) {
    for (std::size_t i = 0; i < triggers_.size(); ++i) {
        const auto& trigger = triggers_[i];
        if (trigger.type != map::TriggerType::Press && trigger.type != map::TriggerType::On &&
            trigger.type != map::TriggerType::Off && trigger.type != map::TriggerType::OnOff) {
            continue;
        }
        if (!trigger.enabled) {
            continue;
        }

        auto& runtime = expander_[i];
        if (runtime.press_time < 0) {
            continue;
        }
        if (runtime.press_time > 0) {
            --runtime.press_time;
            continue;
        }

        if (runtime.press_count >= trigger.press_count_required) {
            apply_expander_effects(i, player, events);
            if (trigger.press_count_required > 0) {
                runtime.press_count -= trigger.press_count_required;
            } else {
                runtime.press_count = 0;
            }
            runtime.press_time = -1;
        }
    }
}

void TriggerSystem::activate_trigger(std::size_t trigger_index, PlayerState& player,
                                     EventBus* events) {
    const auto& trigger = triggers_[trigger_index];

    switch (trigger.type) {
    case map::TriggerType::Exit:
        exit_requested_ = true;
        break;
    case map::TriggerType::Teleport: {
        float tx = static_cast<float>(trigger.teleport_target.x);
        float ty = static_cast<float>(trigger.teleport_target.y);
        if (trigger.d2d) {
            tx -= PlayerState::width * 0.5f;
            ty -= PlayerState::height;
        }
        (void)try_teleport_player(player, tx, ty, events);
        break;
    }
    case map::TriggerType::OpenDoor:
        set_door_group_open(trigger.target_panel, true);
        break;
    case map::TriggerType::CloseDoor:
        set_door_group_open(trigger.target_panel, false);
        break;
    case map::TriggerType::CloseTrap:
        close_trap_group(trigger.target_panel, player, events);
        break;
    case map::TriggerType::Trap:
        close_trap_group(trigger.target_panel, player, events);
        trap_timers_.push_back({trigger.target_panel, kTrapReopenTicks});
        break;
    case map::TriggerType::Door: {
        bool any_enabled = false;
        for (const auto& group : door_groups_) {
            for (const auto idx : group) {
                if (static_cast<std::int32_t>(idx) == trigger.target_panel) {
                    any_enabled = std::any_of(group.begin(), group.end(),
                                              [&](std::size_t i) { return panels_[i].enabled; });
                    set_door_group_open(trigger.target_panel, any_enabled);
                    mark_trigger_used(trigger_index);
                    return;
                }
            }
        }
        if (static_cast<std::size_t>(trigger.target_panel) < panels_.size()) {
            set_door_group_open(trigger.target_panel, panels_[trigger.target_panel].enabled);
        }
        break;
    }
    case map::TriggerType::Door5:
        set_door_group_open(trigger.target_panel, true);
        door5_timers_.push_back({trigger.target_panel, kDoor5CloseTicks});
        break;
    case map::TriggerType::LiftUp:
        lift_.set_group_for_zone(panels_, trigger.target_panel, true);
        break;
    case map::TriggerType::LiftDown:
        lift_.set_group_for_zone(panels_, trigger.target_panel, false);
        break;
    case map::TriggerType::Lift:
        lift_.toggle_group_for_zone(panels_, trigger.target_panel);
        break;
    case map::TriggerType::Press:
    case map::TriggerType::On:
    case map::TriggerType::Off:
    case map::TriggerType::OnOff:
        queue_expander(trigger_index);
        mark_trigger_used(trigger_index);
        return;
    default:
        break;
    }

    mark_trigger_used(trigger_index);
}

void TriggerSystem::mark_trigger_used(std::size_t trigger_index) {
    int timeout = 0;
    switch (triggers_[trigger_index].type) {
    case map::TriggerType::Trap:
        timeout = 76;
        break;
    case map::TriggerType::Exit:
    case map::TriggerType::Press:
    case map::TriggerType::On:
    case map::TriggerType::Off:
    case map::TriggerType::OnOff:
    case map::TriggerType::Door:
    case map::TriggerType::Door5:
        timeout = 18;
        break;
    default:
        break;
    }
    trigger_timeout_[trigger_index] = timeout;
}

void TriggerSystem::apply_on_load_triggers() {
    for (std::size_t i = 0; i < triggers_.size(); ++i) {
        const auto& trigger = triggers_[i];
        if (!trigger.enabled ||
            !map::has_activate_on_load(trigger.activate, trigger.type)) {
            continue;
        }

        PlayerState dummy;
        dummy.snap_to(0.0f, 0.0f);
        activate_trigger(i, dummy, nullptr);
    }
}

void TriggerSystem::update(PlayerState& player, bool use_pressed, EventBus* events,
                           const std::vector<ShootableTarget>* monsters,
                           const MapCollision* collision) {
    active_collision_ = collision;
    tick_door_timers();

    for (std::size_t i = 0; i < triggers_.size(); ++i) {
        if (trigger_timeout_[i] > 0) {
            --trigger_timeout_[i];
        }

        const auto& trigger = triggers_[i];
        if (!trigger.enabled || trigger.type == map::TriggerType::None) {
            continue;
        }

        if (map::has_activate_on_load(trigger.activate, trigger.type)) {
            continue;
        }

        if (trigger_timeout_[i] > 0) {
            continue;
        }

        const bool has_monster_collide =
            has_flag(trigger.activate, map::ActivateType::MonsterCollide);
        const bool has_no_monster = has_flag(trigger.activate, map::ActivateType::NoMonster);

        if (has_monster_collide && has_no_monster && !nomonster_boot_done_[i] &&
            trigger.keys == 0) {
            nomonster_boot_done_[i] = true;
            try_activate_trigger(i, player, events, monsters);
            continue;
        }

        if (has_no_monster && !has_monster_collide && trigger.keys == 0 &&
            !any_monsters_in_trigger(trigger, monsters)) {
            try_activate_trigger(i, player, events, monsters);
            continue;
        }

        const bool inside = player_overlaps(trigger, player);

        if (has_flag(trigger.activate, map::ActivateType::PlayerCollide) && inside) {
            try_activate_trigger(i, player, events, monsters);
        }

        if (has_flag(trigger.activate, map::ActivateType::PlayerPress) && use_pressed && inside) {
            try_activate_trigger(i, player, events, monsters);
        }
    }

    tick_expanders(player, events);
    active_collision_ = nullptr;
}

map::MapDocument TriggerSystem::map_view(const map::MapDocument& base) const {
    map::MapDocument view = base;
    view.panels = panels_;
    return view;
}

void TriggerSystem::press_shot_line(float x1, float y1, float x2, float y2, PlayerState& player,
                                    EventBus* events,
                                    const std::vector<ShootableTarget>* monsters) {
    for (std::size_t i = 0; i < triggers_.size(); ++i) {
        if (trigger_timeout_[i] > 0) {
            continue;
        }

        const auto& trigger = triggers_[i];
        if (!trigger.enabled || trigger.type == map::TriggerType::None) {
            continue;
        }
        if (!has_flag(trigger.activate, map::ActivateType::Shot)) {
            continue;
        }

        const float tx = static_cast<float>(trigger.position.x);
        const float ty = static_cast<float>(trigger.position.y);
        const float tw = static_cast<float>(trigger.size.width);
        const float th = static_cast<float>(trigger.size.height);
        if (!segment_intersects_aabb(x1, y1, x2, y2, tx, ty, tw, th)) {
            continue;
        }

        try_activate_trigger(i, player, events, monsters);
    }
}

void TriggerSystem::press_shot_rect(float x, float y, float width, float height,
                                   PlayerState& player, EventBus* events,
                                   const std::vector<ShootableTarget>* monsters) {
    for (std::size_t i = 0; i < triggers_.size(); ++i) {
        if (trigger_timeout_[i] > 0) {
            continue;
        }

        const auto& trigger = triggers_[i];
        if (!trigger.enabled || trigger.type == map::TriggerType::None) {
            continue;
        }
        if (!has_flag(trigger.activate, map::ActivateType::Shot)) {
            continue;
        }

        const float tx = static_cast<float>(trigger.position.x);
        const float ty = static_cast<float>(trigger.position.y);
        const float tw = static_cast<float>(trigger.size.width);
        const float th = static_cast<float>(trigger.size.height);
        if (!rects_overlap(x, y, width, height, tx, ty, tw, th)) {
            continue;
        }

        try_activate_trigger(i, player, events, monsters);
    }
}

void TriggerSystem::press_shot_circle(float cx, float cy, float radius, PlayerState& player,
                                      EventBus* events,
                                      const std::vector<ShootableTarget>* monsters) {
    if (radius <= 0.0f) {
        return;
    }

    const float radius_sq = radius * radius;
    const float box_x = cx - radius;
    const float box_y = cy - radius;
    const float box_w = radius * 2.0f;
    const float box_h = radius * 2.0f;

    for (std::size_t i = 0; i < triggers_.size(); ++i) {
        if (trigger_timeout_[i] > 0) {
            continue;
        }

        const auto& trigger = triggers_[i];
        if (!trigger.enabled || trigger.type == map::TriggerType::None) {
            continue;
        }
        if (!has_flag(trigger.activate, map::ActivateType::Shot)) {
            continue;
        }

        const float tx = static_cast<float>(trigger.position.x);
        const float ty = static_cast<float>(trigger.position.y);
        const float tw = static_cast<float>(trigger.size.width);
        const float th = static_cast<float>(trigger.size.height);
        if (!rects_overlap(box_x, box_y, box_w, box_h, tx, ty, tw, th)) {
            continue;
        }

        const float corners[4][2] = {{tx, ty},
                                     {tx + tw, ty},
                                     {tx + tw, ty + th},
                                     {tx, ty + th}};
        bool near_corner = false;
        for (const auto& corner : corners) {
            const float dx = cx - corner[0];
            const float dy = cy - corner[1];
            if (dx * dx + dy * dy < radius_sq) {
                near_corner = true;
                break;
            }
        }

        if (!near_corner) {
            const float closest_x = std::clamp(cx, tx, tx + tw);
            const float closest_y = std::clamp(cy, ty, ty + th);
            const float dx = cx - closest_x;
            const float dy = cy - closest_y;
            if (dx * dx + dy * dy >= radius_sq) {
                continue;
            }
        }

        try_activate_trigger(i, player, events, monsters);
    }
}

} // namespace d2df::sim
