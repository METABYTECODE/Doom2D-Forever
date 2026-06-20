#include <rivet/game/level_scene.hpp>

namespace rivet::game {

LevelScene::LevelScene(level::LevelData data, level::LevelSpawnContext spawn_context)
    : Scene(data.name.empty() ? "level" : data.name)
    , data_(std::move(data))
    , spawn_context_(std::move(spawn_context)) {}

void LevelScene::on_enter() {
    spawn_ = level::spawn_level(world(), data_, spawn_context_);
}

} // namespace rivet::game
