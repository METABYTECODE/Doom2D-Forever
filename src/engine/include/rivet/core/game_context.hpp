#pragma once

namespace rivet::core {

class Application;
class Clock;
class FixedTimestep;

} // namespace rivet::core

namespace rivet::input {
class InputSystem;
}

namespace rivet::mod {
class Api;
}

namespace rivet::scene {
class SceneManager;
}

namespace rivet::core {

class ServiceRegistry;

/// Stable game-facing API surface. Game code should depend on this instead of Application.
class GameContext {
public:
    explicit GameContext(Application& app);

    [[nodiscard]] scene::SceneManager& scenes();
    [[nodiscard]] const scene::SceneManager& scenes() const;
    [[nodiscard]] input::InputSystem& input();
    [[nodiscard]] const input::InputSystem& input() const;
    [[nodiscard]] ServiceRegistry& services();
    [[nodiscard]] const ServiceRegistry& services() const;
    [[nodiscard]] mod::Api& mod_api();
    [[nodiscard]] const mod::Api& mod_api() const;
    [[nodiscard]] Clock& clock();
    [[nodiscard]] const Clock& clock() const;
    [[nodiscard]] FixedTimestep& fixed_timestep();
    [[nodiscard]] const FixedTimestep& fixed_timestep() const;
    [[nodiscard]] bool is_running() const;

    void request_quit();

    template <typename T>
    [[nodiscard]] bool has_service() const {
        return services().has<T>();
    }

    template <typename T>
    [[nodiscard]] T& service() {
        return services().get<T>();
    }

    template <typename T>
    [[nodiscard]] const T& service() const {
        return services().get<T>();
    }

private:
    Application* app_ = nullptr;
};

} // namespace rivet::core
