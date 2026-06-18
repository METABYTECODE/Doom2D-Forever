#pragma once

#include <string_view>

#include <rivet/core/application.hpp>
#include <rivet/platform/window_config.hpp>
#include <rivet/render/renderer_backend.hpp>

namespace rivet::platform {

struct BootstrapConfig {
    WindowConfig window{};
    render::RendererBackend renderer = render::RendererBackend::Sdl;
};

/// Installs platform + renderer (via factory) + resources on the application.
void install_backends(core::Application& app, const BootstrapConfig& config = {});

[[nodiscard]] BootstrapConfig parse_bootstrap_config(int argc, char** argv);

} // namespace rivet::platform
