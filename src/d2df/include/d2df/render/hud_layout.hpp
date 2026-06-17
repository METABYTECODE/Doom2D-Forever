#pragma once

#include <algorithm>

#include <d2df/sim/weapon_types.hpp>

namespace d2df::render {

/// Legacy reserves a 196px-wide HUD strip on the right (`gScreenWidth - 196`).
struct HudLayout {
    static constexpr int kStripWidth = 196;
    static constexpr int kPanelHeight = 256;
    static constexpr int kTextureSize = 256;
    static constexpr int kStripOffsetX = 2;

    static constexpr int kHealthTextX = 178;
    static constexpr int kHealthTextY = 40;
    static constexpr int kArmorTextY = 68;
    static constexpr int kAmmoTextY = 158;

    static constexpr int kHealthIconX = 37;
    static constexpr int kHealthIconY = 45;
    static constexpr int kArmorIconX = 36;
    static constexpr int kArmorIconY = 77;
    static constexpr int kWeaponIconX = 20;
    static constexpr int kWeaponIconY = 160;

    static constexpr int kKeyRedX = 78;
    static constexpr int kKeyGreenX = 95;
    static constexpr int kKeyBlueX = 112;
    static constexpr int kKeyY = 214;

    static constexpr int kAirBarX = 2;
    static constexpr int kAirBarY = 124;
    static constexpr int kAirBarWithJetY = 116;
    static constexpr int kJetBarX = 2;
    static constexpr int kJetBarY = 126;
    static constexpr int kBarFillX = 16;
    static constexpr int kAirFillY = 130;
    static constexpr int kAirFillWithJetY = 122;
    static constexpr int kJetFillY = 132;
    static constexpr int kBarFillMaxWidth = 168;
};

struct HudMetrics {
    int strip_x = 0;
    int strip_width = HudLayout::kStripWidth;
    int panel_y = 0;
    int panel_height = HudLayout::kPanelHeight;
    float x_scale = static_cast<float>(HudLayout::kStripWidth) /
                    static_cast<float>(HudLayout::kTextureSize);

    [[nodiscard]] int sx(int legacy_x) const {
        return strip_x + HudLayout::kStripOffsetX +
               static_cast<int>(static_cast<float>(legacy_x) * x_scale);
    }

    [[nodiscard]] int sy(int legacy_y) const { return panel_y + legacy_y; }

    [[nodiscard]] int sw(int legacy_width) const {
        return std::max(1, static_cast<int>(static_cast<float>(legacy_width) * x_scale));
    }
};

[[nodiscard]] HudMetrics hud_metrics(int viewport_w, int viewport_h);

[[nodiscard]] int game_viewport_width(int viewport_w);

[[nodiscard]] const char* weapon_hud_icon(sim::WeaponId weapon);

[[nodiscard]] bool weapon_hud_uses_ammo_text(sim::WeaponId weapon);

} // namespace d2df::render
