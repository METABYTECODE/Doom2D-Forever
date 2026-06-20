#pragma once

#include <rivet/game/level/level_data.hpp>
#include <rivet/game/level/level_spawner.hpp>
#include <rivet/scene/scene.hpp>

namespace rivet::game {

class LevelScene final : public rivet::scene::Scene {
public:
    explicit LevelScene(level::LevelData data, level::LevelSpawnContext spawn_context = {});

    [[nodiscard]] const level::LevelData& data() const { return data_; }
    [[nodiscard]] rivet::ecs::Entity player_entity() const { return spawn_.player; }
    [[nodiscard]] const std::vector<rivet::ecs::Entity>& patrol_entities() const { return spawn_.patrols; }
    [[nodiscard]] float world_width() const { return spawn_.world_width; }
    [[nodiscard]] float world_height() const { return spawn_.world_height; }

protected:
    void on_enter() override;

private:
    level::LevelData data_;
    level::LevelSpawnContext spawn_context_;
    level::LevelSpawnResult spawn_;
};

} // namespace rivet::game
