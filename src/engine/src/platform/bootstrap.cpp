#include <rivet/audio/audio_system.hpp>
#include <rivet/platform/bootstrap.hpp>

#include <memory>

#include <spdlog/spdlog.h>
#include <rivet/platform/sdl_platform.hpp>
#include <rivet/physics/physics_world.hpp>
#include <rivet/render/irenderer.hpp>
#include <rivet/render/renderer_factory.hpp>
#include <rivet/resources/resource_manager.hpp>

namespace rivet::platform {

namespace {

[[nodiscard]] std::string_view value_after_equals(std::string_view arg) {
    const auto pos = arg.find('=');
    if (pos == std::string_view::npos) {
        return {};
    }
    return arg.substr(pos + 1);
}

} // namespace

void install_backends(core::Application& app, const BootstrapConfig& config) {
    auto platform = std::make_shared<SdlPlatform>(config.window);
    const render::RendererCreateInfo renderer_info{.window = platform->sdl_window()};
    auto renderer = render::create_renderer(config.renderer, renderer_info);
    auto resources = std::make_shared<resources::ResourceManager>();
    resources->bind_renderer(renderer.get());

    app.services().register_service<Platform>(platform);
    app.services().register_service<render::IRenderer>(renderer);
    app.services().register_service<resources::ResourceManager>(resources);
    app.services().register_service<physics::PhysicsWorld>(std::make_shared<physics::PhysicsWorld>());

    auto audio = std::make_shared<audio::AudioSystem>();
    if (!audio->init()) {
        spdlog::warn("Audio backend unavailable; continuing without sound");
    }
    app.services().register_service<audio::AudioSystem>(audio);
}

BootstrapConfig parse_bootstrap_config(const int argc, char** argv) {
    BootstrapConfig config;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg.starts_with("--renderer=")) {
            config.renderer = render::parse_renderer_backend(value_after_equals(arg));
        } else if (arg == "--renderer" && i + 1 < argc) {
            config.renderer = render::parse_renderer_backend(argv[++i]);
        }
    }
    return config;
}

} // namespace rivet::platform
