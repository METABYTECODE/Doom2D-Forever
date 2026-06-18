#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

#include <rivet/core/game.hpp>

namespace rivet::game {

enum class GameId {
    Sandbox,
    Bounce,
    Level,
};

struct LaunchOptions {
    GameId game = GameId::Level;
    std::filesystem::path level_path = std::filesystem::path("assets") / "levels" / "test.level.json";
};

[[nodiscard]] std::string_view to_string(GameId id);
[[nodiscard]] GameId parse_game_id(std::string_view value);
[[nodiscard]] std::unique_ptr<rivet::core::Game> create_game(const LaunchOptions& options);

} // namespace rivet::game
