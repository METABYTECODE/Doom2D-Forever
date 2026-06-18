#include <rivet/scene/scene_manager.hpp>

namespace rivet::scene {

void SceneManager::push(std::unique_ptr<Scene> scene) {
    if (scene == nullptr) {
        return;
    }
    if (!scenes_.empty()) {
        scenes_.back()->exit();
    }
    scenes_.push_back(std::move(scene));
    scenes_.back()->enter();
}

void SceneManager::pop() {
    if (scenes_.empty()) {
        return;
    }
    scenes_.back()->exit();
    scenes_.pop_back();
    if (!scenes_.empty()) {
        scenes_.back()->enter();
    }
}

void SceneManager::replace(std::unique_ptr<Scene> scene) {
    if (!scenes_.empty()) {
        scenes_.back()->exit();
        scenes_.pop_back();
    }
    push(std::move(scene));
}

void SceneManager::clear() {
    while (!scenes_.empty()) {
        pop();
    }
}

Scene* SceneManager::active() {
    return scenes_.empty() ? nullptr : scenes_.back().get();
}

const Scene* SceneManager::active() const {
    return scenes_.empty() ? nullptr : scenes_.back().get();
}

void SceneManager::update(float delta_time) {
    if (Scene* scene = active()) {
        scene->update(delta_time);
    }
}

void SceneManager::fixed_update(float fixed_delta_time) {
    if (Scene* scene = active()) {
        scene->fixed_update(fixed_delta_time);
    }
}

} // namespace rivet::scene
