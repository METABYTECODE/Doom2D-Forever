#pragma once

#include <d2df/debug/debug_panel.hpp>

namespace d2df::debug {

class CollisionDebugPanel final : public IDebugPanel {
public:
    [[nodiscard]] const char* title() const override { return "Collision"; }
    void draw_ui(DebugContext& context) override;
    void draw_world(SDL_Renderer* renderer, const render::Camera2D& camera, int viewport_w,
                    int viewport_h, const DebugWorldView& world,
                    const DebugContext& context) const override;
};

} // namespace d2df::debug
