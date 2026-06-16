#include <d2df/app/game_app.hpp>

#include <chrono>
#include <filesystem>

#include <SDL.h>
#include <spdlog/spdlog.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <d2df/app/content_paths.hpp>
#include <d2df/app/map_viewer.hpp>
#include <d2df/core/event_bus.hpp>
#include <d2df/core/game_events.hpp>
#include <d2df/core/log.hpp>
#include <d2df/core/service_registry.hpp>
#include <d2df/net/local_simulation_authority.hpp>
#include <d2df/net/network_transport.hpp>
#include <d2df/net/null_network_transport.hpp>
#include <d2df/net/simulation_authority.hpp>

namespace d2df {

GameApp::GameApp(GameAppConfig config)
    : config_(std::move(config))
    , services_(std::make_unique<ServiceRegistry>())
    , events_(std::make_unique<EventBus>()) {}

GameApp::~GameApp() {
    shutdown_sdl();
}

bool GameApp::init_sdl() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        spdlog::error("SDL_Init failed: {}", SDL_GetError());
        return false;
    }

    window_ = SDL_CreateWindow(config_.window_title.c_str(), SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED, config_.window_width, config_.window_height,
                               SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (!window_) {
        spdlog::error("SDL_CreateWindow failed: {}", SDL_GetError());
        return false;
    }

    const Uint32 renderer_flags =
        SDL_RENDERER_ACCELERATED | (config_.vsync ? SDL_RENDERER_PRESENTVSYNC : 0);

    renderer_ = SDL_CreateRenderer(window_, -1, renderer_flags);
    if (!renderer_) {
        spdlog::error("SDL_CreateRenderer failed: {}", SDL_GetError());
        return false;
    }

    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    spdlog::info("Window {}x{}", config_.window_width, config_.window_height);
    return true;
}

void GameApp::shutdown_sdl() {
    map_viewer_.reset();
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
}

void GameApp::register_core_services() {
    services_->register_service<ISimulationAuthority>(
        std::make_shared<LocalSimulationAuthority>());
    services_->register_service<INetworkTransport>(std::make_shared<NullNetworkTransport>());
    spdlog::debug("Services: LocalSimulationAuthority, NullNetworkTransport");
}

namespace {

void show_fatal_error(const char* message) {
    spdlog::error("{}", message);
#ifdef _WIN32
    MessageBoxA(nullptr, message,
                "Doom2D Forever", MB_OK | MB_ICONERROR);
#endif
}

std::filesystem::path map_relative_from_config(const std::filesystem::path& map_path) {
    const auto generic = map_path.generic_string();
    const std::string content_prefix = "assets/content/";
    if (generic.rfind(content_prefix, 0) == 0) {
        return std::filesystem::path(generic.substr(content_prefix.size()));
    }
    if (map_path.is_relative()) {
        return map_path;
    }
    return std::filesystem::path("maps/doom2d/map01.json");
}

} // namespace

bool GameApp::resolve_asset_paths() {
    const auto manifest = config_.content_root / "manifest.json";
    if (std::filesystem::exists(manifest) && std::filesystem::exists(config_.map_path)) {
        spdlog::info("Content: {}", config_.content_root.generic_string());
        spdlog::info("Map: {}", config_.map_path.generic_string());
        return true;
    }

    const auto map_rel = map_relative_from_config(config_.map_path);
    const auto resolved = resolve_content_paths(map_rel);
    if (!resolved) {
        show_fatal_error(
            "Could not find assets/content/manifest.json.\n\n"
            "Run from the project root, use run_game.bat, or pass:\n"
            "  d2df.exe --content PATH --map PATH");
        return false;
    }

    config_.content_root = resolved->content_root;
    if (!std::filesystem::exists(config_.map_path)) {
        config_.map_path = resolved->map_path;
    }

    spdlog::info("Content: {}", config_.content_root.generic_string());
    spdlog::info("Map: {}", config_.map_path.generic_string());
    return true;
}

bool GameApp::init_map_viewer() {
    try {
        map_viewer_ = std::make_unique<MapViewer>(renderer_, config_.content_root, config_.map_path,
                                                  events_.get());
        return true;
    } catch (const std::exception& ex) {
        show_fatal_error(ex.what());
        return false;
    }
}

void GameApp::process_frame() {
    const auto now = std::chrono::steady_clock::now();
    const float dt = std::chrono::duration<float>(now - last_tick_).count();
    last_tick_ = now;

    services_->get<INetworkTransport>().poll();
    events_->publish(events::FrameTick{dt});

    int viewport_w = config_.window_width;
    int viewport_h = config_.window_height;
    SDL_GetRendererOutputSize(renderer_, &viewport_w, &viewport_h);

    if (map_viewer_) {
        map_viewer_->update(dt);
        map_viewer_->render(viewport_w, viewport_h);
    } else {
        SDL_SetRenderDrawColor(renderer_, 20, 15, 26, 255);
        SDL_RenderClear(renderer_);
    }

    SDL_RenderPresent(renderer_);
}

int GameApp::run() {
    init_logging();
    spdlog::info("Doom2D Forever — Phase 5 (combat MVP)");

    if (!init_sdl()) {
        return 1;
    }

    if (!resolve_asset_paths()) {
        return 1;
    }

    register_core_services();

    if (!init_map_viewer()) {
        return 1;
    }

    events_->publish(events::AppStarted{});

    running_ = true;
    last_tick_ = std::chrono::steady_clock::now();

    while (running_) {
        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running_ = false;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running_ = false;
            } else if (event.type == SDL_KEYDOWN && map_viewer_) {
                map_viewer_->handle_key_down(event.key.keysym.sym, event.key.keysym.scancode);
            } else if (event.type == SDL_KEYUP && map_viewer_) {
                map_viewer_->handle_key_up(event.key.keysym.sym);
            } else if (event.type == SDL_MOUSEWHEEL && map_viewer_) {
                map_viewer_->handle_mouse_wheel(event.wheel.y);
            }
        }

        if (!running_) {
            break;
        }

        process_frame();
    }

    events_->publish(events::AppShutdown{});
    spdlog::info("Shutdown");
    return 0;
}

} // namespace d2df
