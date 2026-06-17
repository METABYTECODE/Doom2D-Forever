#include <d2df/render/hud_layout.hpp>

#include <d2df/sim/weapon_types.hpp>

namespace d2df::render {

HudMetrics hud_metrics(int viewport_w, int viewport_h) {
    HudMetrics metrics{};
    metrics.strip_x = viewport_w - HudLayout::kStripWidth;
    metrics.panel_y = std::max(0, viewport_h - HudLayout::kPanelHeight);
    return metrics;
}

int game_viewport_width(int viewport_w) {
    return std::max(1, viewport_w - HudLayout::kStripWidth);
}

const char* weapon_hud_icon(sim::WeaponId weapon) {
    switch (weapon) {
    case sim::WeaponId::Knuckles:
        return "tex.ui.knuckles";
    case sim::WeaponId::Saw:
        return "tex.ui.saw";
    case sim::WeaponId::Pistol:
        return "tex.ui.pistol";
    case sim::WeaponId::Shotgun1:
        return "tex.ui.shotgun1";
    case sim::WeaponId::Shotgun2:
        return "tex.ui.shotgun2";
    case sim::WeaponId::Chaingun:
        return "tex.ui.mgun_2";
    case sim::WeaponId::RocketLauncher:
        return "tex.ui.rlauncher";
    case sim::WeaponId::Plasma:
        return "tex.ui.pgun";
    case sim::WeaponId::Bfg:
        return "tex.ui.bfg_2";
    case sim::WeaponId::SuperChaingun:
        return "tex.ui.schaingun";
    case sim::WeaponId::Flamethrower:
        return "tex.ui.flamethrower";
    default:
        return nullptr;
    }
}

bool weapon_hud_uses_ammo_text(sim::WeaponId weapon) {
    switch (weapon) {
    case sim::WeaponId::Knuckles:
    case sim::WeaponId::Saw:
        return false;
    default:
        return true;
    }
}

} // namespace d2df::render
