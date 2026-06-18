#include <rivet/core/application.hpp>

#include <rivet/core/game_context.hpp>
#include <rivet/platform/platform.hpp>
#include <rivet/render/irenderer.hpp>

namespace rivet::core {

Application::Application() = default;

Application::~Application() {
    if (game_ != nullptr) {
        GameContext context(*this);
        game_->on_detach(context);
        game_ = nullptr;
    }
    if (initialized_) {
        on_shutdown();
    }
}

void Application::request_quit() {
    running_ = false;
}

void Application::attach_game(Game& game) {
    GameContext context(*this);
    if (game_ != nullptr && game_ != &game) {
        game_->on_detach(context);
    }
    game_ = &game;
    game.on_attach(context);
}

void Application::detach_game() {
    if (game_ == nullptr) {
        return;
    }
    GameContext context(*this);
    game_->on_detach(context);
    game_ = nullptr;
}

void Application::run() {
    while (is_running()) {
        run_one_frame(1.0f / 60.0f);
    }
}

void Application::run(Game& game) {
    attach_game(game);
    run();
    detach_game();
}

void Application::run_one_frame(float frame_dt_seconds) {
    if (!initialized_) {
        on_init();
        initialized_ = true;
    }

    if (!running_) {
        return;
    }

    input_.begin_frame();

    float delta_time = frame_dt_seconds;
    if (services_.has<platform::Platform>()) {
        auto& platform = services_.get<platform::Platform>();
        platform.poll_events(input_);
        delta_time = platform.frame_delta_seconds();
        if (platform.should_close() || input_.state().quit_requested) {
            running_ = false;
            return;
        }
    }

    input_.update_axes();

    clock_.tick(delta_time);
    fixed_timestep_.advance(delta_time);

    GameContext context(*this);
    if (game_ != nullptr) {
        game_->on_update(context, clock_.delta_time());
    } else {
        on_update(clock_.delta_time());
    }
    scenes_.update(clock_.delta_time());

    while (fixed_timestep_.consume_fixed_step()) {
        if (game_ != nullptr) {
            game_->on_fixed_update(context, fixed_timestep_.step_seconds());
        } else {
            on_fixed_update(fixed_timestep_.step_seconds());
        }
        scenes_.fixed_update(fixed_timestep_.step_seconds());

        mod::Event tick_event;
        tick_event.name = "OnTick";
        tick_event.data["dt"] = static_cast<double>(fixed_timestep_.step_seconds());
        mod_api_.emit(tick_event);
    }

    mod_api_.flush_commands();

    const float render_alpha = fixed_timestep_.render_alpha();
    if (services_.has<render::IRenderer>()) {
        auto& renderer = services_.get<render::IRenderer>();
        renderer.begin_frame();
        if (game_ != nullptr) {
            game_->on_render(context, render_alpha);
        } else {
            on_render(render_alpha);
        }
        renderer.end_frame();
    } else if (game_ != nullptr) {
        game_->on_render(context, render_alpha);
    } else {
        on_render(render_alpha);
    }
}

} // namespace rivet::core
