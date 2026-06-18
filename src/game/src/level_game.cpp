#include <rivet/game/level_game.hpp>
#include <rivet/game/level_scene.hpp>

#include <rivet/ecs/components/collider.hpp>
#include <rivet/ecs/components/transform.hpp>
#include <rivet/game/level/level_loader.hpp>
#include <rivet/input/keys.hpp>

namespace rivet::game {

namespace {

constexpr int kSolidTile = 1;
constexpr float kMoveSpeed = 280.0f;

[[nodiscard]] LevelScene* active_level(rivet::core::GameContext& context) {
    return static_cast<LevelScene*>(context.scenes().active());
}

[[nodiscard]] rivet::render::Color tile_color(const int tile_id) {
    if (tile_id == kSolidTile) {
        return {.r = 70, .g = 78, .b = 96, .a = 255};
    }
    return {.r = 28, .g = 32, .b = 42, .a = 255};
}

} // namespace

LevelGame::LevelGame(std::filesystem::path level_path)
    : level_path_(std::move(level_path)) {}

void LevelGame::on_attach(rivet::core::GameContext& context) {
    level_ = level::load_level(level_path_);
    context.scenes().push(std::make_unique<LevelScene>(level_));

    if (auto* scene = active_level(context)) {
        float bounds_margin_x = 48.0f;
        float bounds_margin_y = 48.0f;
        if (scene->player_entity() != rivet::ecs::kNullEntity) {
            const auto& collider = scene->world().registry().get<rivet::ecs::components::Collider>(
                scene->player_entity());
            bounds_margin_x = collider.width;
            bounds_margin_y = collider.height;
        }
        physics_.set_world_bounds({
            .min_x = 0.0f,
            .min_y = 0.0f,
            .max_x = scene->world_width() - bounds_margin_x,
            .max_y = scene->world_height() - bounds_margin_y,
            .enabled = true,
        });
    }

    if (context.has_service<rivet::resources::ResourceManager>()) {
        auto& resources = context.service<rivet::resources::ResourceManager>();
        player_texture_ = resources.create_checkerboard(64, 8, "player");
    }

    if (context.has_service<rivet::render::IRenderer>()) {
        auto& renderer = context.service<rivet::render::IRenderer>();
        rivet::render::Camera2D camera;
        camera.set_viewport(800, 600);
        renderer.set_camera(camera);
    }
}

void LevelGame::on_detach(rivet::core::GameContext& context) {
    (void)context;
    player_texture_ = rivet::resources::kInvalidTexture;
    physics_.clear();
}

void LevelGame::on_update(rivet::core::GameContext& context, float delta_time) {
    (void)delta_time;
    if (context.input().is_key_pressed(rivet::input::Key::Escape)) {
        context.request_quit();
    }

    auto* scene = active_level(context);
    if (scene != nullptr && scene->player_entity() != rivet::ecs::kNullEntity) {
        auto& velocity = scene->world().registry().get<rivet::ecs::components::Velocity>(
            scene->player_entity());
        velocity.x = context.input().state().move_x * kMoveSpeed;
        velocity.y = context.input().state().move_y * kMoveSpeed;
    }

    if (!context.has_service<rivet::render::IRenderer>()) {
        return;
    }

    if (scene == nullptr || scene->player_entity() == rivet::ecs::kNullEntity) {
        return;
    }

    const auto& transform = scene->world().registry().get<rivet::ecs::components::Transform>(
        scene->player_entity());
    const auto& collider = scene->world().registry().get<rivet::ecs::components::Collider>(
        scene->player_entity());

    auto& renderer = context.service<rivet::render::IRenderer>();
    auto camera = renderer.camera();
    camera.follow(
        transform.x + collider.width * 0.5f,
        transform.y + collider.height * 0.5f,
        0.12f);
    renderer.set_camera(camera);
}

void LevelGame::on_fixed_update(rivet::core::GameContext& context, float fixed_delta_time) {
    auto* scene = active_level(context);
    if (scene == nullptr) {
        return;
    }
    physics_.step(scene->world(), fixed_delta_time);
}

void LevelGame::on_render(rivet::core::GameContext& context, float interpolation_alpha) {
    (void)interpolation_alpha;
    if (!context.has_service<rivet::render::IRenderer>()) {
        return;
    }

    auto& renderer = context.service<rivet::render::IRenderer>();
    renderer.clear({.r = 18, .g = 20, .b = 28, .a = 255});

    const auto* scene = active_level(context);
    if (scene == nullptr) {
        return;
    }

    const float tile_size = static_cast<float>(scene->data().tile_size);
    for (int y = 0; y < scene->data().height; ++y) {
        for (int x = 0; x < scene->data().width; ++x) {
            const int tile_id = scene->data().tiles[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
            if (tile_id == 0) {
                continue;
            }
            renderer.draw_filled_rect(
                {
                    .x = static_cast<float>(x) * tile_size,
                    .y = static_cast<float>(y) * tile_size,
                    .width = tile_size,
                    .height = tile_size,
                },
                tile_color(tile_id));
        }
    }

    if (scene->player_entity() == rivet::ecs::kNullEntity) {
        return;
    }

    const auto& transform =
        scene->world().registry().get<rivet::ecs::components::Transform>(scene->player_entity());
    const auto& collider =
        scene->world().registry().get<rivet::ecs::components::Collider>(scene->player_entity());

    if (player_texture_ != rivet::resources::kInvalidTexture) {
        renderer.draw_texture(
            player_texture_,
            {.x = transform.x, .y = transform.y, .width = collider.width, .height = collider.height});
    } else {
        renderer.draw_filled_rect(
            {.x = transform.x, .y = transform.y, .width = collider.width, .height = collider.height},
            {.r = 220, .g = 90, .b = 70, .a = 255});
    }
}

} // namespace rivet::game
