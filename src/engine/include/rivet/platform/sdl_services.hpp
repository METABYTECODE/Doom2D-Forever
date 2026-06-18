#pragma once

#include <rivet/platform/bootstrap.hpp>

namespace rivet::platform {

/// Backward-compatible entry point. Prefer install_backends().
inline void install_sdl_services(core::Application& app, const WindowConfig& config = {}) {
    BootstrapConfig bootstrap;
    bootstrap.window = config;
    install_backends(app, bootstrap);
}

} // namespace rivet::platform
