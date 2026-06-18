#include <d2df/engine/physics/entity_collision.hpp>

#include <algorithm>
#include <cmath>

namespace d2df::engine::physics {
namespace {

bool rects_overlap(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

} // namespace

void resolve_player_entity_collisions(sim::PlayerState& player, float entity_x, float entity_y,
                                      float entity_width, float entity_height) {
    if (entity_width <= 0.0f || entity_height <= 0.0f) {
        return;
    }

    const float player_left = player.x;
    const float player_top = player.y;
    const float player_right = player.x + player.width;
    const float player_bottom = player.y + player.height;

    const float entity_left = entity_x;
    const float entity_top = entity_y;
    const float entity_right = entity_x + entity_width;
    const float entity_bottom = entity_y + entity_height;

    if (!rects_overlap(player_left, player_top, player.width, player.height, entity_left, entity_top,
                       entity_width, entity_height)) {
        return;
    }

    const float overlap_left = player_right - entity_left;
    const float overlap_right = entity_right - player_left;
    const float overlap_top = player_bottom - entity_top;
    const float overlap_bottom = entity_bottom - player_top;

    const float min_x_overlap = std::min(overlap_left, overlap_right);
    const float min_y_overlap = std::min(overlap_top, overlap_bottom);

    if (min_x_overlap < min_y_overlap) {
        if (overlap_left < overlap_right) {
            player.x -= overlap_left;
        } else {
            player.x += overlap_right;
        }
    } else {
        if (overlap_top < overlap_bottom) {
            player.y -= overlap_top;
        } else {
            player.y += overlap_bottom;
        }
    }
}

} // namespace d2df::engine::physics
