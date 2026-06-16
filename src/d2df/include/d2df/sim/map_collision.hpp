#pragma once

#include <d2df/map/map_document.hpp>

#include <cstdint>
#include <vector>

namespace d2df::sim {

inline constexpr std::uint16_t MOVE_NONE = 0;
inline constexpr std::uint16_t MOVE_HITWALL = 1;
inline constexpr std::uint16_t MOVE_HITCEIL = 2;
inline constexpr std::uint16_t MOVE_HITLAND = 4;
inline constexpr std::uint16_t MOVE_INWATER = 16;
inline constexpr std::uint16_t MOVE_HITWATER = 32;
inline constexpr std::uint16_t MOVE_HITAIR = 64;
inline constexpr std::uint16_t MOVE_BLOCK = 128;

struct RayHit {
    bool hit = false;
    float x = 0.0f;
    float y = 0.0f;
    float distance_sq = 0.0f;
};

class MapCollision {
public:
    void build_from_map(const map::MapDocument& map);
    void build_from_panels(const std::vector<map::MapPanel>& panels);

    [[nodiscard]] bool collides_panel(float x, float y, float width, float height,
                                      std::uint16_t mask) const;
    [[nodiscard]] bool on_ground(float x, float y, float width, float height) const;
    [[nodiscard]] bool in_liquid(float x, float y, float width, float height) const;
    [[nodiscard]] bool in_water(float x, float y, float width, float height) const;
    [[nodiscard]] bool in_acid(float x, float y, float width, float height) const;
    /// Legacy g_GetAcidHit: 0, 5, 10, or 20 depending on acid panel overlap.
    [[nodiscard]] int acid_damage(float x, float y, float width, float height) const;
    [[nodiscard]] bool overlaps_solid(float x, float y, float width, float height) const;

    /// -1 = lift up, 0 = none, 1 = lift down (legacy CollideLift).
    [[nodiscard]] int vertical_lift_at(float x, float y, float width, float height) const;
    /// -1 = lift left, 0 = none, 1 = lift right (legacy CollideHorLift).
    [[nodiscard]] int horizontal_lift_at(float x, float y, float width, float height) const;

    [[nodiscard]] std::uint16_t move_object(float& x, float& y, float width, float height, int dx,
                                            int dy, bool climb_slopes) const;
    [[nodiscard]] std::uint16_t move_monster(float& x, float& y, float width, float height, int dx,
                                             int dy, bool climb_slopes) const;

    /// Ray trace against solid panels (walls + closed doors). Distance along segment [0,1].
    [[nodiscard]] RayHit trace_solid_ray(float x0, float y0, float x1, float y1) const;

    [[nodiscard]] const std::vector<map::MapPanel>& panels() const { return panels_; }

private:
    [[nodiscard]] bool segment_intersects_aabb(float x0, float y0, float x1, float y1, float bx,
                                               float by, float bw, float bh, float& hit_x,
                                               float& hit_y, float& hit_t) const;
    [[nodiscard]] bool stay_on_step(float x, float y, float width, float height) const;
    [[nodiscard]] bool can_move_y(float x, float y, float width, float height, int ystep) const;
    [[nodiscard]] bool move_axis_x(float& xtemp, float& ytemp, float width, float height, int xstep,
                                   int ystep, std::uint16_t& state, bool climb_slopes,
                                   bool block_monsters) const;
    [[nodiscard]] bool move_axis_y(float& xtemp, float& ytemp, float width, float height, int xstep,
                                   int ystep, std::uint16_t& state, bool block_monsters) const;
    [[nodiscard]] bool is_solid_panel(const map::MapPanel& panel, std::size_t panel_index) const;
    [[nodiscard]] std::uint16_t move_body(float& x, float& y, float width, float height, int dx,
                                          int dy, bool climb_slopes, bool block_monsters) const;

    std::vector<map::MapPanel> panels_;
};

} // namespace d2df::sim
