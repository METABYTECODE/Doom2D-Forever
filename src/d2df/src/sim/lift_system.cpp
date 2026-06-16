#include <d2df/sim/lift_system.hpp>

#include <d2df/map/panel_types.hpp>
#include <d2df/map/trigger_types.hpp>
#include <spdlog/spdlog.h>

namespace d2df::sim {
namespace {

bool is_lift_zone_type(std::uint16_t type) {
    return (type & (map::PANEL_LIFTUP | map::PANEL_LIFTDOWN | map::PANEL_LIFTLEFT |
                    map::PANEL_LIFTRIGHT)) != 0;
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

} // namespace

void LiftSystem::reset(const std::vector<map::MapPanel>& panels) {
    panels_ = panels;
    groups_.clear();
    build_groups();
}

void LiftSystem::apply_on_load(const std::vector<map::MapTrigger>& triggers) {
    for (const auto& trigger : triggers) {
        if (!trigger.enabled || trigger.type == map::TriggerType::None ||
            !map::has_activate_on_load(trigger.activate)) {
            continue;
        }

        const bool up = trigger.type != map::TriggerType::LiftDown;
        if (trigger.type == map::TriggerType::LiftUp || trigger.type == map::TriggerType::LiftDown) {
            set_group_for_zone(trigger.target_panel, up);
        }
    }

    spdlog::info("LiftSystem: {} lift groups", groups_.size());
}

void LiftSystem::build_groups() {
    std::vector<std::size_t> lift_indices;
    for (std::size_t i = 0; i < panels_.size(); ++i) {
        const auto& panel = panels_[i];
        if ((panel.flags & map::PANEL_FLAG_HIDE) != 0 || !is_lift_zone_type(panel.type)) {
            continue;
        }
        lift_indices.push_back(i);
    }

    std::vector<bool> assigned(lift_indices.size(), false);
    for (std::size_t seed = 0; seed < lift_indices.size(); ++seed) {
        if (assigned[seed]) {
            continue;
        }

        LiftGroup group;
        group.zone_panels.push_back(lift_indices[seed]);
        assigned[seed] = true;

        bool expanded = true;
        while (expanded) {
            expanded = false;
            for (std::size_t candidate = 0; candidate < lift_indices.size(); ++candidate) {
                if (assigned[candidate]) {
                    continue;
                }

                const auto& candidate_panel = panels_[lift_indices[candidate]];
                for (const auto zone_index : group.zone_panels) {
                    if (panels_touch(candidate_panel, panels_[zone_index])) {
                        group.zone_panels.push_back(lift_indices[candidate]);
                        assigned[candidate] = true;
                        expanded = true;
                        break;
                    }
                }
            }
        }

        groups_.push_back(std::move(group));
    }
}

void LiftSystem::set_group_direction(LiftGroup& group, bool up) {
    for (const auto zone_index : group.zone_panels) {
        auto& zone = panels_[zone_index];
        if (up) {
            zone.type = (zone.type & ~(map::PANEL_LIFTDOWN | map::PANEL_LIFTLEFT |
                                       map::PANEL_LIFTRIGHT)) |
                        map::PANEL_LIFTUP;
        } else {
            zone.type = (zone.type & ~(map::PANEL_LIFTUP | map::PANEL_LIFTLEFT |
                                       map::PANEL_LIFTRIGHT)) |
                        map::PANEL_LIFTDOWN;
        }
    }
}

void LiftSystem::toggle_group_for_zone(std::int32_t zone_panel_index) {
    for (auto& group : groups_) {
        for (const auto zone_index : group.zone_panels) {
            if (static_cast<std::int32_t>(zone_index) != zone_panel_index) {
                continue;
            }
            const bool up = (panels_[zone_index].type & map::PANEL_LIFTDOWN) != 0;
            set_group_direction(group, up);
            return;
        }
    }
}

void LiftSystem::set_group_for_zone(std::int32_t zone_panel_index, bool up) {
    for (auto& group : groups_) {
        for (const auto zone_index : group.zone_panels) {
            if (static_cast<std::int32_t>(zone_index) == zone_panel_index) {
                set_group_direction(group, up);
                return;
            }
        }
    }
}

} // namespace d2df::sim
