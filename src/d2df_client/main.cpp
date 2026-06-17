#include <d2df/app/game_app.hpp>

#include <iostream>
#include <string>

#if defined(_DEBUG) && defined(D2DF_LEAK_CHECK)
#include <crtdbg.h>
#endif

namespace {

void print_usage() {
    std::cerr << "Usage: d2df [--content PATH] [--map PATH]\n"
              << "  --content  Content root (default: assets/content)\n"
              << "  --map      Map JSON path (skips main menu; default: maps/doom2d/map01.json)\n"
              << "Controls: A/D move, Space jump, PgUp/PgDn or [/] maps, C camera, +/- zoom, ESC quit\n"
              << "Debug: Insert toggles ImGui overlay (Performance tab for FPS profiling)\n";
}

#if defined(_DEBUG) && defined(D2DF_LEAK_CHECK)
void enable_crt_leak_check() {
    int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    flags |= _CRTDBG_LEAK_CHECK_DF;
    _CrtSetDbgFlag(flags);
}
#endif

} // namespace

extern "C" int SDL_main(int argc, char* argv[]) {
#if defined(_DEBUG) && defined(D2DF_LEAK_CHECK)
    enable_crt_leak_check();
#endif

    d2df::GameAppConfig config;
    config.window_title = "Doom2D Forever — Map Viewer";

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_usage();
            return 0;
        }
        if (arg == "--content" && i + 1 < argc) {
            config.content_root = argv[++i];
        } else if (arg == "--map" && i + 1 < argc) {
            config.map_path = argv[++i];
            config.skip_main_menu = true;
        } else {
            std::cerr << "Unknown argument: " << arg << '\n';
            print_usage();
            return 1;
        }
    }

    d2df::GameApp app{config};
    return app.run();
}
