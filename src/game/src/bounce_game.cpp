#include <cstdint>
#include <memory>
#include <vector>

#include <rivet/game/bounce_game.hpp>
#include <rivet/game/bounce_scene.hpp>

namespace rivet::game {

namespace {

constexpr float kArenaWidth = 800.0f;
constexpr float kArenaHeight = 600.0f;
constexpr float kPlayerSize = 40.0f;
constexpr float kMoveSpeed = 280.0f;

[[nodiscard]] BounceScene* active_bounce(rivet::core::GameContext& context) {
    return static_cast<BounceScene*>(context.scenes().active());
}

[[nodiscard]] rivet::resources::TextureHandle create_stripe_texture(
    rivet::core::GameContext& context) {
    if (!context.has_service<rivet::resources::ResourceManager>()) {
        return rivet::resources::kInvalidTexture;
    }

    const int size = 64;
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(size * size * 4));
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const bool stripe = (x / 8) % 2 == 0;
            const std::size_t index = static_cast<std::size_t>((y * size + x) * 4);
            pixels[index + 0] = stripe ? 70 : 30;
            pixels[index + 1] = stripe ? 200 : 140;
            pixels[index + 2] = stripe ? 220 : 170;
            pixels[index + 3] = 255;
        }
    }

    return context.service<rivet::resources::ResourceManager>().create_texture_rgba(
        pixels,
        size,
        size,
        "bounce_stripes");
}

} // namespace

void BounceGame::on_attach(rivet::core::GameContext& context) {
    context.scenes().push(std::make_unique<BounceScene>());

    physics_.set_world_bounds({
        .min_x = 0.0f,
        .min_y = 0.0f,
        .max_x = kArenaWidth - kPlayerSize,
        .max_y = kArenaHeight - kPlayerSize,
        .enabled = true,
    });

    player_texture_ = create_stripe_texture(context);

    if (context.has_service<rivet::render::IRenderer>()) {
        auto& render = context.service<rivet::render::IRenderer>();
        rivet::render::Camera2D camera;
        camera.set_viewport(static_cast<int>(kArenaWidth), static_cast<int>(kArenaHeight));
        camera.center_on(kArenaWidth * 0.5f, kArenaHeight * 0.5f);
        render.set_camera(camera);
    }
}

void BounceGame::on_detach(rivet::core::GameContext& context) {
    (void)context;
    player_texture_ = rivet::resources::kInvalidTexture;
    physics_.clear();
}

void BounceGame::on_update(rivet::core::GameContext& context, float delta_time) {
    (void)delta_time;
    if (context.input().is_key_pressed(rivet::input::Key::Escape)) {
        context.request_quit();
    }

    auto* scene = active_bounce(context);
    if (scene == nullptr) {
        return;
    }

    auto& velocity =
        scene->world().registry().get<rivet::ecs::components::Velocity>(scene->player_entity());
    velocity.x = context.input().state().move_x * kMoveSpeed;
    velocity.y = context.input().state().move_y * kMoveSpeed;
}

void BounceGame::on_fixed_update(rivet::core::GameContext& context, float fixed_delta_time) {
    auto* scene = active_bounce(context);
    if (scene == nullptr) {
        return;
    }
    physics_.step(scene->world(), fixed_delta_time);
}

void BounceGame::on_render(rivet::core::GameContext& context, float interpolation_alpha) {
    (void)interpolation_alpha;
    if (!context.has_service<rivet::render::IRenderer>()) {
        return;
    }

    auto& render = context.service<rivet::render::IRenderer>();
    render.clear({.r = 12, .g = 16, .b = 24, .a = 255});

    const auto* scene = active_bounce(context);
    if (scene == nullptr) {
        return;
    }

    render.draw_filled_rect(
        {.x = 0.0f, .y = 0.0f, .width = kArenaWidth, .height = kArenaHeight},
        {.r = 20, .g = 28, .b = 38, .a = 255});

    const auto& transform =
        scene->world().registry().get<rivet::ecs::components::Transform>(scene->player_entity());

    if (player_texture_ != rivet::resources::kInvalidTexture) {
        render.draw_texture(
            player_texture_,
            {.x = transform.x, .y = transform.y, .width = kPlayerSize, .height = kPlayerSize});
    } else {
        render.draw_filled_rect(
            {.x = transform.x, .y = transform.y, .width = kPlayerSize, .height = kPlayerSize},
            {.r = 90, .g = 220, .b = 200, .a = 255});
    }
}

} // namespace rivet::game
