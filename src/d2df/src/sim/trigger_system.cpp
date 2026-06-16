#include <d2df/sim/trigger_system.hpp>

#include <algorithm>

#include <d2df/map/panel_types.hpp>
#include <d2df/map/trigger_types.hpp>
#include <spdlog/spdlog.h>

namespace d2df::sim {
namespace {

constexpr std::uint16_t kDoorMask = map::PANEL_CLOSEDOOR | map::PANEL_OPENDOOR;

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

bool rects_overlap(float ax, float ay, float aw, float ah, float bx, float by, float bw,
                   float bh) {
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

bool has_flag(map::ActivateType flags, map::ActivateType flag) {
    return (static_cast<std::uint8_t>(flags) & static_cast<std::uint8_t>(flag)) != 0;
}

} // namespace

void TriggerSystem::reset(const map::MapDocument& map) {
    source_ = map;
    panels_ = map.panels;
    triggers_ = map.triggers;
    door_groups_.clear();
    trap_timers_.clear();
    collide_was_inside_.assign(triggers_.size(), false);

    init_panel_defaults();
    build_door_groups();
    lift_.reset(panels_);
    lift_.apply_on_load(triggers_);
    panels_ = lift_.panels();
    apply_on_load_triggers();

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

void TriggerSystem::close_trap_group(std::int32_t panel_index) {
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
            if (!panels_[idx].enabled) {
                panels_[idx].enabled = true;
            }
        }
        return;
    }

    if (static_cast<std::size_t>(panel_index) < panels_.size() &&
        !panels_[panel_index].enabled) {
        panels_[panel_index].enabled = true;
    }
}

void TriggerSystem::tick_trap_timers() {
    for (auto& timer : trap_timers_) {
        if (timer.second <= 0) {
            continue;
        }
        --timer.second;
        if (timer.second == 0) {
            set_door_group_open(timer.first, true);
        }
    }
}

bool TriggerSystem::player_overlaps(const map::MapTrigger& trigger,
                                    const PlayerState& player) const {
    return rects_overlap(player.x, player.y, player.width, player.height,
                         static_cast<float>(trigger.position.x),
                         static_cast<float>(trigger.position.y),
                         static_cast<float>(trigger.size.width),
                         static_cast<float>(trigger.size.height));
}

bool TriggerSystem::trigger_affects(const map::MapTrigger& source,
                                    const map::MapTrigger& target) const {
    const float ax = static_cast<float>(source.press_position.x);
    const float ay = static_cast<float>(source.press_position.y);
    const float aw = static_cast<float>(source.press_size.width);
    const float ah = static_cast<float>(source.press_size.height);
    if (aw <= 0 || ah <= 0) {
        return rects_overlap(static_cast<float>(source.position.x),
                             static_cast<float>(source.position.y),
                             static_cast<float>(source.size.width),
                             static_cast<float>(source.size.height),
                             static_cast<float>(target.position.x),
                             static_cast<float>(target.position.y),
                             static_cast<float>(target.size.width),
                             static_cast<float>(target.size.height));
    }

    return rects_overlap(ax, ay, aw, ah, static_cast<float>(target.position.x),
                         static_cast<float>(target.position.y),
                         static_cast<float>(target.size.width),
                         static_cast<float>(target.size.height));
}

void TriggerSystem::activate_trigger(std::size_t trigger_index, PlayerState& player) {
    const auto& trigger = triggers_[trigger_index];

    switch (trigger.type) {
    case map::TriggerType::Teleport: {
        float tx = static_cast<float>(trigger.teleport_target.x);
        float ty = static_cast<float>(trigger.teleport_target.y);
        if (trigger.d2d) {
            tx -= PlayerState::width * 0.5f;
            ty -= PlayerState::height;
        }
        player.snap_to(tx, ty);
        break;
    }
    case map::TriggerType::OpenDoor:
        set_door_group_open(trigger.target_panel, true);
        break;
    case map::TriggerType::CloseDoor:
        set_door_group_open(trigger.target_panel, false);
        break;
    case map::TriggerType::CloseTrap:
        close_trap_group(trigger.target_panel);
        break;
    case map::TriggerType::Trap:
        close_trap_group(trigger.target_panel);
        trap_timers_.push_back({trigger.target_panel, 40});
        break;
    case map::TriggerType::Door:
    case map::TriggerType::Door5: {
        bool any_enabled = false;
        for (const auto& group : door_groups_) {
            for (const auto idx : group) {
                if (static_cast<std::int32_t>(idx) == trigger.target_panel) {
                    any_enabled = std::any_of(group.begin(), group.end(),
                                              [&](std::size_t i) { return panels_[i].enabled; });
                    set_door_group_open(trigger.target_panel, any_enabled);
                    return;
                }
            }
        }
        if (static_cast<std::size_t>(trigger.target_panel) < panels_.size()) {
            set_door_group_open(trigger.target_panel, panels_[trigger.target_panel].enabled);
        }
        break;
    }
    case map::TriggerType::LiftUp:
        lift_.set_group_for_zone(trigger.target_panel, true);
        panels_ = lift_.panels();
        break;
    case map::TriggerType::LiftDown:
        lift_.set_group_for_zone(trigger.target_panel, false);
        panels_ = lift_.panels();
        break;
    case map::TriggerType::Lift:
        lift_.toggle_group_for_zone(trigger.target_panel);
        panels_ = lift_.panels();
        break;
    case map::TriggerType::Press:
    case map::TriggerType::On:
    case map::TriggerType::Off:
    case map::TriggerType::OnOff:
        for (std::size_t i = 0; i < triggers_.size(); ++i) {
            if (i == trigger_index || !triggers_[i].enabled) {
                continue;
            }
            if (!trigger_affects(trigger, triggers_[i])) {
                continue;
            }

            switch (trigger.type) {
            case map::TriggerType::Press:
                activate_trigger(i, player);
                break;
            case map::TriggerType::On:
                triggers_[i].enabled = true;
                break;
            case map::TriggerType::Off:
                triggers_[i].enabled = false;
                break;
            case map::TriggerType::OnOff:
                triggers_[i].enabled = !triggers_[i].enabled;
                break;
            default:
                break;
            }
        }
        break;
    default:
        break;
    }
}

void TriggerSystem::apply_on_load_triggers() {
    for (std::size_t i = 0; i < triggers_.size(); ++i) {
        const auto& trigger = triggers_[i];
        if (!trigger.enabled || !map::has_activate_on_load(trigger.activate)) {
            continue;
        }

        PlayerState dummy;
        dummy.snap_to(0.0f, 0.0f);
        activate_trigger(i, dummy);
    }
}

void TriggerSystem::update(PlayerState& player, bool use_pressed) {
    tick_trap_timers();

    for (std::size_t i = 0; i < triggers_.size(); ++i) {
        const auto& trigger = triggers_[i];
        if (!trigger.enabled || trigger.type == map::TriggerType::None) {
            continue;
        }

        if (map::has_activate_on_load(trigger.activate)) {
            continue;
        }

        const bool inside = player_overlaps(trigger, player);

        if (has_flag(trigger.activate, map::ActivateType::PlayerCollide)) {
            if (!inside) {
                collide_was_inside_[i] = false;
            } else if (!collide_was_inside_[i]) {
                collide_was_inside_[i] = true;
                activate_trigger(i, player);
            }
        }

        if (has_flag(trigger.activate, map::ActivateType::PlayerPress) && use_pressed && inside) {
            activate_trigger(i, player);
        }
    }
}

map::MapDocument TriggerSystem::map_view(const map::MapDocument& base) const {
    map::MapDocument view = base;
    view.panels = panels_;
    return view;
}

} // namespace d2df::sim
