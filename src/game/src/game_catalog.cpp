#include <rivet/game/bounce_game.hpp>
#include <rivet/game/game_catalog.hpp>
#include <rivet/game/level_game.hpp>
#include <rivet/game/sandbox_game.hpp>

namespace rivet::game {

std::string_view to_string(const GameId id) {
    switch (id) {
    case GameId::Sandbox:
        return "sandbox";
    case GameId::Bounce:
        return "bounce";
    case GameId::Level:
        return "level";
    }
    return "level";
}

GameId parse_game_id(const std::string_view value) {
    if (value == "sandbox") {
        return GameId::Sandbox;
    }
    if (value == "bounce") {
        return GameId::Bounce;
    }
    return GameId::Level;
}

std::unique_ptr<rivet::core::Game> create_game(const LaunchOptions& options) {
    switch (options.game) {
    case GameId::Bounce:
        return std::make_unique<BounceGame>();
    case GameId::Sandbox:
        return std::make_unique<SandboxGame>();
    case GameId::Level:
    default:
        return std::make_unique<LevelGame>(options.level_path);
    }
}

} // namespace rivet::game
