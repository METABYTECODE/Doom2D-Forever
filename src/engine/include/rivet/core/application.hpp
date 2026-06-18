#pragma once

#include <rivet/core/clock.hpp>
#include <rivet/core/event_bus.hpp>
#include <rivet/core/fixed_timestep.hpp>
#include <rivet/core/game.hpp>
#include <rivet/core/service_registry.hpp>
#include <rivet/input/input_system.hpp>
#include <rivet/mod/api.hpp>
#include <rivet/scene/scene_manager.hpp>

namespace rivet::core {

/// Top-level engine runtime: clock, fixed timestep, scenes, services.
/// Game code subclasses this and wires platform/render backends via services.
class Application {
public:
    Application();
    virtual ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void run_one_frame(float frame_dt_seconds);
    void run();
    void run(Game& game);
    void attach_game(Game& game);
    void detach_game();
    void request_quit();

    [[nodiscard]] Clock& clock() { return clock_; }
    [[nodiscard]] const Clock& clock() const { return clock_; }
    [[nodiscard]] FixedTimestep& fixed_timestep() { return fixed_timestep_; }
    [[nodiscard]] const FixedTimestep& fixed_timestep() const { return fixed_timestep_; }
    [[nodiscard]] scene::SceneManager& scenes() { return scenes_; }
    [[nodiscard]] const scene::SceneManager& scenes() const { return scenes_; }
    [[nodiscard]] ServiceRegistry& services() { return services_; }
    [[nodiscard]] const ServiceRegistry& services() const { return services_; }
    [[nodiscard]] EventBus& events() { return events_; }
    [[nodiscard]] const EventBus& events() const { return events_; }
    [[nodiscard]] mod::Api& mod_api() { return mod_api_; }
    [[nodiscard]] const mod::Api& mod_api() const { return mod_api_; }
    [[nodiscard]] input::InputSystem& input() { return input_; }
    [[nodiscard]] const input::InputSystem& input() const { return input_; }
    [[nodiscard]] bool is_running() const { return running_; }

protected:
    virtual void on_init() {}
    virtual void on_shutdown() {}
    virtual void on_update(float delta_time) { (void)delta_time; }
    virtual void on_fixed_update(float fixed_delta_time) { (void)fixed_delta_time; }
    virtual void on_render(float interpolation_alpha) { (void)interpolation_alpha; }

private:
    bool running_ = true;
    bool initialized_ = false;
    Clock clock_;
    FixedTimestep fixed_timestep_;
    scene::SceneManager scenes_;
    ServiceRegistry services_;
    EventBus events_;
    mod::Api mod_api_;
    input::InputSystem input_;
    Game* game_ = nullptr;
};

} // namespace rivet::core
