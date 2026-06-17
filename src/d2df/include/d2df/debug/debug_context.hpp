#pragma once

namespace d2df::debug {

struct DebugContext {
    bool menu_visible = false;

    struct CollisionFlags {
        bool player = false;
        bool monsters = false;
        bool projectiles = false;
    } collision;

    struct TileFlags {
        bool solid = false;
        bool steps = false;
        bool water = false;
        bool acid = false;
        bool doors = false;
        bool lifts = false;
    } tiles;

    [[nodiscard]] bool any_collision_overlay() const {
        return collision.player || collision.monsters || collision.projectiles;
    }

    [[nodiscard]] bool any_tile_overlay() const {
        return tiles.solid || tiles.steps || tiles.water || tiles.acid || tiles.doors ||
               tiles.lifts;
    }

    [[nodiscard]] bool any_world_overlay() const {
        return any_collision_overlay() || any_tile_overlay();
    }
};

} // namespace d2df::debug
