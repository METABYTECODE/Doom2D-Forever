#include <rivet/game/level_game.hpp>
#include <rivet/game/level_scene.hpp>

#include <string>

#include <rivet/ecs/components/collider.hpp>
#include <rivet/ecs/components/transform.hpp>
#include <rivet/game/level/level_loader.hpp>
#include <rivet/input/keys.hpp>

namespace rivet::game {

namespace {

constexpr float kMoveSpeed = 280.0f;

[[nodiscard]] LevelScene* active_level(rivet::core::GameContext& context) {
    return static_cast<LevelScene*>(context.scenes().active());
}

[[nodiscard]] rivet::render::Color placement_color(const std::string& tileset, const int tile_id) {
    const std::size_t hash = std::hash<std::string>{}(tileset) ^ static_cast<std::size_t>(tile_id * 131);
    return {
        .r = static_cast<std::uint8_t>(60 + (hash % 120)),
        .g = static_cast<std::uint8_t>(60 + ((hash / 7) % 120)),
        .b = static_cast<std::uint8_t>(60 + ((hash / 49) % 120)),
        .a = 255,
    };
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

    const float cell_size = static_cast<float>(scene->data().grid_size);
    for (int y = 0; y < scene->data().height; ++y) {
        for (int x = 0; x < scene->data().width; ++x) {
            if (scene->data().collision[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] == 0) {
                continue;
            }
            renderer.draw_filled_rect(
                {
                    .x = static_cast<float>(x) * cell_size,
                    .y = static_cast<float>(y) * cell_size,
                    .width = cell_size,
                    .height = cell_size,
                },
                {.r = 70, .g = 78, .b = 96, .a = 255});
        }
    }

    for (const auto& tile : scene->data().tiles) {
        renderer.draw_filled_rect(
            {
                .x = static_cast<float>(tile.x) * cell_size,
                .y = static_cast<float>(tile.y) * cell_size,
                .width = cell_size,
                .height = cell_size,
            },
            placement_color(tile.tileset, tile.id));
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
