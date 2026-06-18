#include <rivet/scene/scene.hpp>

namespace rivet::scene {

Scene::Scene(std::string name)
    : name_(std::move(name)) {}

void Scene::enter() {
    if (active_) {
        return;
    }
    active_ = true;
    on_enter();
}

void Scene::exit() {
    if (!active_) {
        return;
    }
    on_exit();
    active_ = false;
}

void Scene::update(float delta_time) {
    if (!active_) {
        return;
    }
    on_update(delta_time);
}

void Scene::fixed_update(float fixed_delta_time) {
    if (!active_) {
        return;
    }
    on_fixed_update(fixed_delta_time);
}

} // namespace rivet::scene
