#pragma once

#include <functional>
#include <memory>
#include <string>

#include <rivet/ecs/world.hpp>

namespace rivet::scene {

/// One logical world slice: ECS registry + lifecycle hooks.
/// A level, menu, or loading screen is typically a Scene.
class Scene {
public:
    explicit Scene(std::string name);
    virtual ~Scene() = default;

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    [[nodiscard]] const std::string& name() const { return name_; }
    [[nodiscard]] bool is_active() const { return active_; }
    [[nodiscard]] ecs::World& world() { return world_; }
    [[nodiscard]] const ecs::World& world() const { return world_; }

    void enter();
    void exit();
    void update(float delta_time);
    void fixed_update(float fixed_delta_time);

protected:
    virtual void on_enter() {}
    virtual void on_exit() {}
    virtual void on_update(float delta_time) { (void)delta_time; }
    virtual void on_fixed_update(float fixed_delta_time) { (void)fixed_delta_time; }

private:
    std::string name_;
    bool active_ = false;
    ecs::World world_;
};

} // namespace rivet::scene
