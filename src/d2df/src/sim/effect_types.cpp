#include <d2df/sim/effect_types.hpp>

namespace d2df::sim {
namespace {

ExplosionSprite make_sprite(const char* texture_id, int frame_w, int frame_h, int frame_count,
                            int anim_period, float offset_x = 0.0f, float offset_y = 0.0f) {
    return {texture_id, frame_w, frame_h, frame_count, anim_period, offset_x, offset_y};
}

} // namespace

ExplosionSprite explosion_sprite(events::ExplosionKind kind) {
    switch (kind) {
    case events::ExplosionKind::Rocket:
        return make_sprite("tex.ui.erocket", 128, 128, 6, 6, -64.0f, -64.0f);
    case events::ExplosionKind::SkelFire:
        return make_sprite("tex.ui.eskelfire", 64, 64, 3, 8, -32.0f, -32.0f);
    case events::ExplosionKind::Bfg:
        return make_sprite("tex.ui.ebfg", 128, 128, 6, 6, -64.0f, -64.0f);
    case events::ExplosionKind::Plasma:
        return make_sprite("tex.ui.eplasma", 32, 32, 4, 3, -16.0f, -16.0f);
    case events::ExplosionKind::BspFire:
        return make_sprite("tex.ui.ebspfire", 32, 32, 5, 3, -16.0f, -16.0f);
    case events::ExplosionKind::ImpFire:
        return make_sprite("tex.ui.eimpfire", 64, 64, 3, 6, -32.0f, -32.0f);
    case events::ExplosionKind::CacoFire:
        return make_sprite("tex.ui.ecacofire", 64, 64, 3, 6, -32.0f, -32.0f);
    case events::ExplosionKind::BaronFire:
        return make_sprite("tex.ui.ebaronfire", 64, 64, 3, 6, -32.0f, -32.0f);
    case events::ExplosionKind::MancubFire:
        return make_sprite("tex.ui.erocket", 128, 128, 6, 6, -64.0f, -64.0f);
    default:
        return {};
    }
}

std::optional<events::ExplosionKind> explosion_kind_for_projectile(ProjectileKind kind) {
    switch (kind) {
    case ProjectileKind::Plasma:
        return events::ExplosionKind::Plasma;
    case ProjectileKind::ImpFire:
        return events::ExplosionKind::ImpFire;
    case ProjectileKind::CacoFire:
        return events::ExplosionKind::CacoFire;
    case ProjectileKind::BaronFire:
        return events::ExplosionKind::BaronFire;
    case ProjectileKind::BspFire:
        return events::ExplosionKind::BspFire;
    case ProjectileKind::MancubFire:
        return events::ExplosionKind::MancubFire;
    default:
        return std::nullopt;
    }
}

const char* explosion_sfx(events::ExplosionKind kind) {
    switch (kind) {
    case events::ExplosionKind::Rocket:
    case events::ExplosionKind::SkelFire:
        return "sfx.world.exploderocket";
    case events::ExplosionKind::Plasma:
    case events::ExplosionKind::BspFire:
        return "sfx.world.explodeplasma";
    case events::ExplosionKind::Bfg:
        return "sfx.world.explodebfg";
    case events::ExplosionKind::ImpFire:
    case events::ExplosionKind::CacoFire:
    case events::ExplosionKind::BaronFire:
    case events::ExplosionKind::MancubFire:
        return "sfx.world.explodeball";
    default:
        return nullptr;
    }
}

} // namespace d2df::sim
