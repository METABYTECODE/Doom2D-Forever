#pragma once

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

#if D2DF_DEBUG_UI
namespace d2df::debug {
class DebugUi;
} // namespace d2df::debug
#endif

struct SDL_Window;
struct SDL_Renderer;

namespace d2df {

class EventBus;
class MainMenu;
class MapViewer;
class ServiceRegistry;

struct GameAppConfig {
    std::string window_title = "Doom2D Forever";
    int window_width = 1280;
    int window_height = 720;
    bool vsync = true;
    bool skip_main_menu = false;
    std::filesystem::path content_root = "assets/content";
    std::filesystem::path map_path = "assets/content/maps/doom2d/map01.json";
};

class GameApp {
public:
    explicit GameApp(GameAppConfig config = {});
    ~GameApp();

    GameApp(const GameApp&) = delete;
    GameApp& operator=(const GameApp&) = delete;

    [[nodiscard]] ServiceRegistry& services() { return *services_; }
    [[nodiscard]] EventBus& events() { return *events_; }

    int run();

private:
    bool init_sdl();
    void shutdown_sdl();
    void register_core_services();
    bool resolve_asset_paths();
    bool init_map_viewer();
    bool start_map(const std::filesystem::path& map_path);
    void show_main_menu();
    void process_frame();

    GameAppConfig config_;
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;

    std::unique_ptr<ServiceRegistry> services_;
    std::unique_ptr<EventBus> events_;
    std::unique_ptr<MainMenu> main_menu_;
    std::unique_ptr<MapViewer> map_viewer_;

#if D2DF_DEBUG_UI
    std::unique_ptr<debug::DebugUi> debug_ui_;
#endif

    bool running_ = false;
    std::chrono::steady_clock::time_point last_tick_;
};

} // namespace d2df
