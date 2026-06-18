#include <rivet/game/level_scene.hpp>

namespace rivet::game {

LevelScene::LevelScene(level::LevelData data)
    : Scene(data.name.empty() ? "level" : data.name)
    , data_(std::move(data)) {}

void LevelScene::on_enter() {
    spawn_ = level::spawn_level(world(), data_);
}

} // namespace rivet::game
