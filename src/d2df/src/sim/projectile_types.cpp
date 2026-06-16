#include <d2df/sim/projectile_types.hpp>

namespace d2df::sim {
namespace {

ProjectileSprite make_sprite(const char* texture_id, int frame_w = 0, int frame_h = 0,
                             int frame_count = 1, int anim_period = 0, float offset_x = 0.0f,
                             float offset_y = 0.0f, bool rotate = false) {
    return {texture_id, frame_w, frame_h, frame_count, anim_period, offset_x, offset_y, rotate};
}

} // namespace

ProjectileSprite projectile_sprite(ProjectileKind kind) {
    switch (kind) {
    case ProjectileKind::Bullet:
        return make_sprite("tex.ui.ebullet");
    case ProjectileKind::ShotgunPellet:
        return make_sprite("tex.ui.eshell");
    case ProjectileKind::Plasma:
        return make_sprite("tex.ui.bplasma", 16, 16, 2, 5);
    case ProjectileKind::Rocket:
        return make_sprite("tex.ui.brocket", 0, 0, 1, 0, 0.0f, 0.0f, true);
    case ProjectileKind::Bfg:
        return make_sprite("tex.ui.bbfg", 64, 64, 2, 6, -6.0f, -7.0f);
    case ProjectileKind::ImpFire:
        return make_sprite("tex.ui.bimpfire", 16, 16, 2, 4);
    case ProjectileKind::CacoFire:
        return make_sprite("tex.ui.bcacofire", 16, 16, 2, 4);
    case ProjectileKind::BaronFire:
        return make_sprite("tex.ui.bbaronfire", 64, 16, 2, 4, 0.0f, 0.0f, true);
    case ProjectileKind::BspFire:
        return make_sprite("tex.ui.bbspfire", 16, 16, 2, 4);
    case ProjectileKind::SkelFire:
        return make_sprite("tex.ui.bskelfire", 64, 16, 2, 5, 0.0f, 0.0f, true);
    case ProjectileKind::MancubFire:
        return make_sprite("tex.ui.bmancubfire", 64, 32, 2, 4, 0.0f, 0.0f, true);
    case ProjectileKind::Flame:
        return make_sprite("tex.ui.flame", 32, 32, 11, 3);
    default:
        return {};
    }
}

} // namespace d2df::sim
