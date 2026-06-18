#include <iostream>
#include <memory>
#include <string_view>

#include <rivet/api.hpp>
#include <rivet/game/game_catalog.hpp>

namespace {

[[nodiscard]] bool is_option_token(std::string_view arg) {
    return arg.starts_with("--renderer") || arg.starts_with("--level");
}

[[nodiscard]] std::string_view value_after_equals(std::string_view arg) {
    const auto pos = arg.find('=');
    if (pos == std::string_view::npos) {
        return {};
    }
    return arg.substr(pos + 1);
}

[[nodiscard]] rivet::game::LaunchOptions parse_launch_options(int argc, char** argv) {
    rivet::game::LaunchOptions options;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg.starts_with("--renderer")) {
            if (arg == "--renderer" && i + 1 < argc) {
                ++i;
            }
            continue;
        }
        if (arg.starts_with("--level=")) {
            options.level_path = value_after_equals(arg);
            options.game = rivet::game::GameId::Level;
            continue;
        }
        if (arg == "--level" && i + 1 < argc) {
            options.level_path = argv[++i];
            options.game = rivet::game::GameId::Level;
            continue;
        }
        if (arg.starts_with("--game=")) {
            arg.remove_prefix(7);
            options.game = rivet::game::parse_game_id(arg);
            continue;
        }
        if (!arg.starts_with("--")) {
            if (arg.ends_with(".json") || arg.ends_with(".level")) {
                options.level_path = std::string(arg);
                options.game = rivet::game::GameId::Level;
            } else {
                options.game = rivet::game::parse_game_id(arg);
            }
        }
    }

    return options;
}

} // namespace

int main(int argc, char** argv) {
    rivet::core::init_logging();

    auto bootstrap = rivet::platform::parse_bootstrap_config(argc, argv);
    bootstrap.window.title = "Rivet Engine";
    bootstrap.window.width = 800;
    bootstrap.window.height = 600;

    const auto launch = parse_launch_options(argc, argv);
    std::cout << "Renderer: " << rivet::render::to_string(bootstrap.renderer) << '\n';
    std::cout << "Launching game: " << rivet::game::to_string(launch.game) << '\n';
    if (launch.game == rivet::game::GameId::Level) {
        std::cout << "Level: " << launch.level_path.string() << '\n';
    }

    rivet::core::Application app;
    rivet::platform::install_backends(app, bootstrap);

    auto game = rivet::game::create_game(launch);
    app.run(*game);

    return 0;
}
