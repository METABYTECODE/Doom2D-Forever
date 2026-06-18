#pragma once

struct SDL_Window;

#include <rivet/platform/platform.hpp>
#include <rivet/platform/window_config.hpp>

namespace rivet::platform {

/// SDL platform backend: window, events, timing. Rendering lives in rivet::render backends.
class SdlPlatform final : public Platform {
public:
    explicit SdlPlatform(WindowConfig config);
    ~SdlPlatform() override;

    SdlPlatform(const SdlPlatform&) = delete;
    SdlPlatform& operator=(const SdlPlatform&) = delete;

    [[nodiscard]] PlatformInfo info() const override;
    void poll_events(input::InputSystem& input) override;
    [[nodiscard]] bool should_close() const override;
    [[nodiscard]] float frame_delta_seconds() const override;

    [[nodiscard]] SDL_Window* sdl_window() const { return window_; }
    [[nodiscard]] int width() const { return width_; }
    [[nodiscard]] int height() const { return height_; }

private:
    SDL_Window* window_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    bool close_requested_ = false;
    float frame_delta_seconds_ = 1.0f / 60.0f;
    std::uint64_t last_counter_ = 0;
};

} // namespace rivet::platform
