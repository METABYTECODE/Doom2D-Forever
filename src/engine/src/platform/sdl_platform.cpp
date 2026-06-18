#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <stdexcept>
#include <string>

#include <rivet/input/input_system.hpp>
#include <rivet/platform/sdl_platform.hpp>

namespace rivet::platform {

namespace {

[[nodiscard]] input::Key map_scancode(const SDL_Scancode scancode) {
    switch (scancode) {
    case SDL_SCANCODE_ESCAPE:
        return input::Key::Escape;
    case SDL_SCANCODE_LEFT:
        return input::Key::Left;
    case SDL_SCANCODE_RIGHT:
        return input::Key::Right;
    case SDL_SCANCODE_UP:
        return input::Key::Up;
    case SDL_SCANCODE_DOWN:
        return input::Key::Down;
    case SDL_SCANCODE_W:
        return input::Key::W;
    case SDL_SCANCODE_A:
        return input::Key::A;
    case SDL_SCANCODE_S:
        return input::Key::S;
    case SDL_SCANCODE_D:
        return input::Key::D;
    case SDL_SCANCODE_SPACE:
        return input::Key::Space;
    default:
        return input::Key::Unknown;
    }
}

void apply_keyboard_event(const SDL_KeyboardEvent& event, input::InputSystem& input) {
    const auto key = map_scancode(event.keysym.scancode);
    if (key == input::Key::Unknown) {
        return;
    }

    input.set_key_down(key, event.type == SDL_KEYDOWN);
}

} // namespace

SdlPlatform::SdlPlatform(const WindowConfig config)
    : width_(config.width)
    , height_(config.height) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }

    std::uint32_t flags = SDL_WINDOW_SHOWN;
    if (config.resizable) {
        flags |= SDL_WINDOW_RESIZABLE;
    }

    window_ = SDL_CreateWindow(
        config.title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        config.width,
        config.height,
        flags);
    if (window_ == nullptr) {
        SDL_Quit();
        throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
    }

    last_counter_ = SDL_GetPerformanceCounter();
}

SdlPlatform::~SdlPlatform() {
    if (window_ != nullptr) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
}

PlatformInfo SdlPlatform::info() const {
    PlatformInfo info;
    info.os_name = "SDL2";
#if SDL_VERSION_ATLEAST(2, 0, 9)
    info.cpu_name = SDL_GetPlatform();
#else
    info.cpu_name = "unknown";
#endif
    return info;
}

void SdlPlatform::poll_events(input::InputSystem& input) {
    const auto now = SDL_GetPerformanceCounter();
    const auto frequency = static_cast<double>(SDL_GetPerformanceFrequency());
    if (last_counter_ != 0 && frequency > 0.0) {
        frame_delta_seconds_ = static_cast<float>((now - last_counter_) / frequency);
    }
    last_counter_ = now;

    if (frame_delta_seconds_ <= 0.0f) {
        frame_delta_seconds_ = 1.0f / 60.0f;
    } else if (frame_delta_seconds_ > 0.25f) {
        frame_delta_seconds_ = 0.25f;
    }

    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
        if (event_hook_) {
            event_hook_(event);
        }

        switch (event.type) {
        case SDL_QUIT:
            close_requested_ = true;
            input.state().quit_requested = true;
            break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            apply_keyboard_event(event.key, input);
            break;
        case SDL_MOUSEMOTION:
            input.set_mouse_position(
                static_cast<float>(event.motion.x),
                static_cast<float>(event.motion.y));
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
            input.set_mouse_position(
                static_cast<float>(event.button.x),
                static_cast<float>(event.button.y));
            const bool down = event.type == SDL_MOUSEBUTTONDOWN;
            input.set_mouse_button(static_cast<int>(event.button.button), down);
            break;
        }
        case SDL_MOUSEWHEEL:
            input.add_mouse_wheel(static_cast<float>(event.wheel.y));
            break;
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                width_ = event.window.data1;
                height_ = event.window.data2;
            }
            break;
        default:
            break;
        }
    }
}

bool SdlPlatform::should_close() const {
    return close_requested_;
}

float SdlPlatform::frame_delta_seconds() const {
    return frame_delta_seconds_;
}

} // namespace rivet::platform
