#pragma once

#include <cstdint>

namespace d2df::sim {

enum class GameMode : std::uint8_t {
    SinglePlayer,
    Coop,
    Deathmatch,
};

struct GameRules {
    GameMode mode = GameMode::SinglePlayer;
    bool weapons_stay = false;

    [[nodiscard]] bool is_multiplayer() const { return mode != GameMode::SinglePlayer; }
    [[nodiscard]] bool shared_keys() const { return mode == GameMode::Coop; }
    [[nodiscard]] bool items_respawn() const { return is_multiplayer(); }
};

} // namespace d2df::sim
