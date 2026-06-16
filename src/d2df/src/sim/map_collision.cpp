#include <d2df/sim/map_collision.hpp>

#include <algorithm>
#include <cmath>

#include <d2df/map/panel_types.hpp>

namespace d2df::sim {
namespace {

constexpr std::uint16_t kLiquidMask =
    map::PANEL_WATER | map::PANEL_ACID1 | map::PANEL_ACID2;

constexpr std::uint16_t kSolidTypeMask = map::PANEL_WALL | map::PANEL_CLOSEDOOR;

bool rects_overlap(float ax, float ay, float aw, float ah, float bx, float by, float bw,
                   float bh) {
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

bool is_hidden_collision_panel(const map::MapPanel& panel) {
    if ((panel.flags & map::PANEL_FLAG_HIDE) == 0) {
        return false;
    }
    return (panel.type & (map::PANEL_CLOSEDOOR | map::PANEL_OPENDOOR)) != 0;
}

bool include_in_collision(const map::MapPanel& panel) {
    if (panel.size.width <= 0 || panel.size.height <= 0) {
        return false;
    }
    if ((panel.flags & map::PANEL_FLAG_HIDE) == 0) {
        return true;
    }
    return is_hidden_collision_panel(panel);
}

int sign_int(int value) {
    if (value > 0) {
        return 1;
    }
    if (value < 0) {
        return -1;
    }
    return 0;
}

} // namespace

void MapCollision::build_from_map(const map::MapDocument& map) {
    build_from_panels(map.panels);
}

void MapCollision::build_from_panels(const std::vector<map::MapPanel>& panels) {
    panels_.clear();
    panels_.reserve(panels.size());

    for (const auto& panel : panels) {
        if (!include_in_collision(panel)) {
            continue;
        }
        panels_.push_back(panel);
    }
}

bool MapCollision::is_solid_panel(const map::MapPanel& panel, std::size_t /*panel_index*/) const {
    if ((panel.type & map::PANEL_WALL) != 0) {
        return true;
    }
    if ((panel.type & map::PANEL_CLOSEDOOR) != 0) {
        return panel.enabled;
    }
    if ((panel.type & map::PANEL_OPENDOOR) != 0) {
        return panel.enabled;
    }
    return false;
}

bool MapCollision::collides_panel(float x, float y, float width, float height,
                                  std::uint16_t mask) const {
    for (std::size_t i = 0; i < panels_.size(); ++i) {
        const auto& panel = panels_[i];
        if ((panel.type & mask) == 0) {
            continue;
        }
        if (rects_overlap(x, y, width, height, static_cast<float>(panel.position.x),
                          static_cast<float>(panel.position.y),
                          static_cast<float>(panel.size.width),
                          static_cast<float>(panel.size.height))) {
            return true;
        }
    }
    return false;
}

bool MapCollision::stay_on_step(float x, float y, float width, float height) const {
    return !collides_panel(x, y + height - 1.0f, width, 1.0f, map::PANEL_STEP) &&
           collides_panel(x, y + height, width, 1.0f, map::PANEL_STEP);
}

bool MapCollision::in_liquid(float x, float y, float width, float height) const {
    return collides_panel(x, y, width, height * 2.0f / 3.0f, kLiquidMask);
}

bool MapCollision::in_water(float x, float y, float width, float height) const {
    return collides_panel(x, y, width, height * 2.0f / 3.0f, map::PANEL_WATER);
}

bool MapCollision::in_acid(float x, float y, float width, float height) const {
    return collides_panel(x, y, width, height, map::PANEL_ACID1 | map::PANEL_ACID2);
}

int MapCollision::acid_damage(float x, float y, float width, float height) const {
    static constexpr int kDamageTable[] = {0, 5, 10, 20};
    const bool acid1 = collides_panel(x, y, width, height, map::PANEL_ACID1);
    const bool acid2 = collides_panel(x, y, width, height, map::PANEL_ACID2);
    const int index = (acid1 ? 1 : 0) | (acid2 ? 2 : 0);
    return kDamageTable[index];
}

bool MapCollision::overlaps_solid(float x, float y, float width, float height) const {
    for (std::size_t i = 0; i < panels_.size(); ++i) {
        const auto& panel = panels_[i];
        if (!is_solid_panel(panel, i)) {
            continue;
        }
        if (rects_overlap(x, y, width, height, static_cast<float>(panel.position.x),
                          static_cast<float>(panel.position.y),
                          static_cast<float>(panel.size.width),
                          static_cast<float>(panel.size.height))) {
            return true;
        }
    }
    return false;
}

int MapCollision::vertical_lift_at(float x, float y, float width, float height) const {
    const bool lift_up = collides_panel(x, y, width, height, map::PANEL_LIFTUP);
    const bool lift_down = collides_panel(x, y, width, height, map::PANEL_LIFTDOWN);
    if (lift_up && !lift_down) {
        return -1;
    }
    if (lift_down && !lift_up) {
        return 1;
    }
    return 0;
}

int MapCollision::horizontal_lift_at(float x, float y, float width, float height) const {
    const bool lift_left = collides_panel(x, y, width, height, map::PANEL_LIFTLEFT);
    const bool lift_right = collides_panel(x, y, width, height, map::PANEL_LIFTRIGHT);
    if (lift_left && !lift_right) {
        return -1;
    }
    if (lift_right && !lift_left) {
        return 1;
    }
    return 0;
}

bool MapCollision::on_ground(float x, float y, float width, float height) const {
    if (stay_on_step(x, y, width, height)) {
        return true;
    }

    for (std::size_t i = 0; i < panels_.size(); ++i) {
        const auto& panel = panels_[i];
        if (!is_solid_panel(panel, i)) {
            continue;
        }
        if (rects_overlap(x, y + 1.0f, width, height, static_cast<float>(panel.position.x),
                          static_cast<float>(panel.position.y),
                          static_cast<float>(panel.size.width),
                          static_cast<float>(panel.size.height))) {
            return true;
        }
    }
    return false;
}

bool MapCollision::can_move_y(float x, float y, float width, float height, int ystep) const {
    for (std::size_t i = 0; i < panels_.size(); ++i) {
        const auto& panel = panels_[i];
        if (!is_solid_panel(panel, i)) {
            continue;
        }
        if (rects_overlap(x, y + static_cast<float>(ystep), width, height,
                          static_cast<float>(panel.position.x),
                          static_cast<float>(panel.position.y),
                          static_cast<float>(panel.size.width),
                          static_cast<float>(panel.size.height))) {
            return false;
        }
    }

    if (ystep > 0 && stay_on_step(x, y, width, height)) {
        return false;
    }
    return true;
}

bool MapCollision::move_axis_x(float& xtemp, float& ytemp, float width, float height, int xstep,
                               int ystep, std::uint16_t& state, bool climb_slopes) const {
    auto collides_solid_at = [&](float px, float py) {
        for (std::size_t i = 0; i < panels_.size(); ++i) {
            const auto& panel = panels_[i];
            if (!is_solid_panel(panel, i)) {
                continue;
            }
            if (rects_overlap(px, py, width, height, static_cast<float>(panel.position.x),
                              static_cast<float>(panel.position.y),
                              static_cast<float>(panel.size.width),
                              static_cast<float>(panel.size.height))) {
                return true;
            }
        }
        return false;
    };

    if (collides_solid_at(xtemp + static_cast<float>(xstep), ytemp)) {
        if (climb_slopes && std::abs(ystep) < 2 &&
            !collides_solid_at(xtemp + static_cast<float>(xstep), ytemp - 12.0f) && ystep >= 0 &&
            !can_move_y(xtemp, ytemp, width, height, ystep)) {
            int climbed = 0;
            while (climbed < 4 && collides_solid_at(xtemp + static_cast<float>(xstep), ytemp)) {
                ytemp -= 1.0f;
                ++climbed;
            }
            xtemp += static_cast<float>(xstep);
            return true;
        }

        state |= MOVE_HITWALL;
        return false;
    }

    if (collides_panel(xtemp + static_cast<float>(xstep), ytemp, width, height, kLiquidMask)) {
        if ((state & MOVE_INWATER) == 0) {
            state |= MOVE_HITWATER;
        }
    } else if ((state & MOVE_INWATER) != 0) {
        state |= MOVE_HITAIR;
    }

    xtemp += static_cast<float>(xstep);
    return true;
}

bool MapCollision::move_axis_y(float& xtemp, float& ytemp, float width, float height, int xstep,
                               int ystep, std::uint16_t& state) const {
    (void)xstep;
    if (!can_move_y(xtemp, ytemp, width, height, ystep)) {
        if (ystep > 0) {
            state |= MOVE_HITLAND;
        } else {
            state |= MOVE_HITCEIL;
        }
        return false;
    }

    if (collides_panel(xtemp, ytemp + static_cast<float>(ystep), width, height, kLiquidMask)) {
        if ((state & MOVE_INWATER) == 0) {
            state |= MOVE_HITWATER;
        }
    } else if ((state & MOVE_INWATER) != 0) {
        state |= MOVE_HITAIR;
    }

    ytemp += static_cast<float>(ystep);
    return true;
}

std::uint16_t MapCollision::move_object(float& x, float& y, float width, float height, int dx,
                                        int dy, bool climb_slopes) const {
    std::uint16_t state = MOVE_NONE;

    if (in_liquid(x, y, width, height)) {
        state |= MOVE_INWATER;
    }

    if (dx == 0 && dy == 0) {
        return state;
    }

    float xtemp = x;
    float ytemp = y;
    const int xstep = sign_int(dx);
    const int ystep = sign_int(dy);

    for (int i = 0; i < std::abs(dx); ++i) {
        if (!move_axis_x(xtemp, ytemp, width, height, xstep, ystep, state, climb_slopes)) {
            break;
        }
        x = xtemp;
    }

    for (int i = 0; i < std::abs(dy); ++i) {
        if (!move_axis_y(xtemp, ytemp, width, height, xstep, ystep, state)) {
            break;
        }
        y = ytemp;
    }

    x = xtemp;
    y = ytemp;
    return state;
}

} // namespace d2df::sim
