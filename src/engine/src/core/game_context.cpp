#include <rivet/core/application.hpp>
#include <rivet/core/game_context.hpp>

namespace rivet::core {

GameContext::GameContext(Application& app)
    : app_(&app) {}

scene::SceneManager& GameContext::scenes() {
    return app_->scenes();
}

const scene::SceneManager& GameContext::scenes() const {
    return app_->scenes();
}

input::InputSystem& GameContext::input() {
    return app_->input();
}

const input::InputSystem& GameContext::input() const {
    return app_->input();
}

ServiceRegistry& GameContext::services() {
    return app_->services();
}

const ServiceRegistry& GameContext::services() const {
    return app_->services();
}

mod::Api& GameContext::mod_api() {
    return app_->mod_api();
}

const mod::Api& GameContext::mod_api() const {
    return app_->mod_api();
}

Clock& GameContext::clock() {
    return app_->clock();
}

const Clock& GameContext::clock() const {
    return app_->clock();
}

FixedTimestep& GameContext::fixed_timestep() {
    return app_->fixed_timestep();
}

const FixedTimestep& GameContext::fixed_timestep() const {
    return app_->fixed_timestep();
}

bool GameContext::is_running() const {
    return app_->is_running();
}

void GameContext::request_quit() {
    app_->request_quit();
}

} // namespace rivet::core
