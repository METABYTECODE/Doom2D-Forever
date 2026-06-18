#pragma once

#include <string>

namespace rivet::input {
class InputSystem;
}

namespace rivet::platform {

struct PlatformInfo {
    std::string os_name;
    std::string cpu_name;
};

/// Abstract platform services: window, timing, OS events.
class Platform {
public:
    virtual ~Platform() = default;

    [[nodiscard]] virtual PlatformInfo info() const = 0;
    virtual void poll_events(input::InputSystem& input) = 0;
    [[nodiscard]] virtual bool should_close() const = 0;
    [[nodiscard]] virtual float frame_delta_seconds() const = 0;
};

} // namespace rivet::platform
