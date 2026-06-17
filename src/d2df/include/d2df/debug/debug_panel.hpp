#pragma once

#include <d2df/debug/debug_context.hpp>
#include <d2df/debug/debug_world_view.hpp>
#include <d2df/render/camera2d.hpp>

struct SDL_Renderer;

namespace d2df::debug {

class IDebugPanel {
public:
    virtual ~IDebugPanel() = default;

    [[nodiscard]] virtual const char* title() const = 0;
    virtual void draw_ui(DebugContext& context) = 0;
    virtual void draw_world(SDL_Renderer* renderer, const render::Camera2D& camera, int viewport_w,
                            int viewport_h, const DebugWorldView& world,
                            const DebugContext& context) const = 0;
};

} // namespace d2df::debug
