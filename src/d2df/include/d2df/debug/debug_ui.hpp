#pragma once

#include <d2df/debug/debug_context.hpp>
#include <d2df/debug/debug_world_view.hpp>
#include <d2df/render/camera2d.hpp>

#include <SDL_events.h>

#include <memory>
#include <vector>

struct SDL_Renderer;
struct SDL_Window;

namespace d2df::debug {

class IDebugPanel;

class DebugUi {
public:
    DebugUi();
    ~DebugUi();

    DebugUi(const DebugUi&) = delete;
    DebugUi& operator=(const DebugUi&) = delete;

    bool init(SDL_Window* window, SDL_Renderer* renderer);
    void shutdown();

    void toggle_menu();
    [[nodiscard]] bool menu_visible() const { return context_.menu_visible; }
    [[nodiscard]] bool wants_capture_keyboard() const;
    [[nodiscard]] bool wants_capture_mouse() const;

    void process_event(const SDL_Event& event);
    void begin_frame();
    void draw_panels();
    void draw_world(SDL_Renderer* renderer, const render::Camera2D& camera, int viewport_w,
                    int viewport_h, const DebugWorldView& world);
    void render();

    [[nodiscard]] const DebugContext& context() const { return context_; }
    DebugContext& context_mut() { return context_; }

private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    bool initialized_ = false;
    DebugContext context_;
    std::vector<std::unique_ptr<IDebugPanel>> panels_;
};

} // namespace d2df::debug
