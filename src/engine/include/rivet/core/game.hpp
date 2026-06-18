#pragma once

namespace rivet::core {

class GameContext;

/// Game-layer contract. Engine calls these hooks; game code owns scenes and rules.
class Game {
public:
    virtual ~Game() = default;

    virtual void on_attach(GameContext& context) = 0;
    virtual void on_detach(GameContext& context) { (void)context; }
    virtual void on_update(GameContext& context, float delta_time) = 0;
    virtual void on_fixed_update(GameContext& context, float fixed_delta_time) = 0;
    virtual void on_render(GameContext& context, float interpolation_alpha) = 0;
};

} // namespace rivet::core
