#pragma once

#include <memory>
#include <vector>

#include <rivet/scene/scene.hpp>

namespace rivet::scene {

/// Owns the active scene stack (push/pop) and forwards ticks to the top scene.
class SceneManager {
public:
    void push(std::unique_ptr<Scene> scene);
    void pop();
    void replace(std::unique_ptr<Scene> scene);
    void clear();

    [[nodiscard]] Scene* active();
    [[nodiscard]] const Scene* active() const;
    [[nodiscard]] std::size_t count() const { return scenes_.size(); }

    void update(float delta_time);
    void fixed_update(float fixed_delta_time);

private:
    std::vector<std::unique_ptr<Scene>> scenes_;
};

} // namespace rivet::scene
