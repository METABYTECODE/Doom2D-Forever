#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

#include <rivet/game/sandbox_game.hpp>
#include <rivet/game/sandbox_scene.hpp>
#include <rivet/physics/physics_world.hpp>

namespace rivet::game {

namespace {

constexpr float kWorldWidth = 1600.0f;
constexpr float kWorldHeight = 1200.0f;
constexpr float kSpriteSize = 48.0f;

[[nodiscard]] std::filesystem::path demo_texture_path() {
    return std::filesystem::path("assets") / "engine" / "generated" / "checker.png";
}

[[nodiscard]] SandboxScene* active_sandbox(rivet::core::GameContext& context) {
    return static_cast<SandboxScene*>(context.scenes().active());
}

void ensure_demo_texture(const std::filesystem::path& path) {
    if (std::filesystem::exists(path)) {
        return;
    }

    const int size = 64;
    const int cell = 8;
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(size * size * 4));
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const bool light = ((x / cell) + (y / cell)) % 2 == 0;
            const std::size_t index = static_cast<std::size_t>((y * size + x) * 4);
            pixels[index + 0] = light ? 230 : 70;
            pixels[index + 1] = light ? 120 : 40;
            pixels[index + 2] = light ? 80 : 20;
            pixels[index + 3] = 255;
        }
    }
    rivet::resources::write_png_rgba(path, pixels, size, size);
}

} // namespace

void SandboxGame::on_attach(rivet::core::GameContext& context) {
    context.scenes().push(std::make_unique<SandboxScene>());

    if (context.has_service<rivet::physics::PhysicsWorld>()) {
        context.service<rivet::physics::PhysicsWorld>().set_world_bounds({
            .min_x = 0.0f,
            .min_y = 0.0f,
            .max_x = kWorldWidth,
            .max_y = kWorldHeight,
            .enabled = true,
        });
    }

    if (context.has_service<rivet::resources::ResourceManager>()) {
        auto& resources = context.service<rivet::resources::ResourceManager>();
        const auto path = demo_texture_path();
        ensure_demo_texture(path);

        player_texture_ = resources.load_texture(path);
        if (player_texture_ == rivet::resources::kInvalidTexture) {
            player_texture_ = resources.create_checkerboard(64, 8, "checkerboard");
        }
    }

    if (context.has_service<rivet::render::IRenderer>()) {
        auto& render = context.service<rivet::render::IRenderer>();
        rivet::render::Camera2D camera;
        camera.set_viewport(800, 600);
        render.set_camera(camera);
    }
}

void SandboxGame::on_detach(rivet::core::GameContext& context) {
    if (context.has_service<rivet::physics::PhysicsWorld>()) {
        context.service<rivet::physics::PhysicsWorld>().clear();
    }
    player_texture_ = rivet::resources::kInvalidTexture;
}

void SandboxGame::on_update(rivet::core::GameContext& context, float delta_time) {
    (void)delta_time;
    if (context.input().is_key_pressed(rivet::input::Key::Escape)) {
        context.request_quit();
    }

    if (!context.has_service<rivet::render::IRenderer>()) {
        return;
    }

    const auto* scene = active_sandbox(context);
    if (scene == nullptr) {
        return;
    }

    const auto& transform = scene->world().registry().get<rivet::ecs::components::Transform>(
        scene->player_entity());

    auto& render = context.service<rivet::render::IRenderer>();
    auto camera = render.camera();
    camera.follow(transform.x + kSpriteSize * 0.5f, transform.y + kSpriteSize * 0.5f, 0.12f);
    render.set_camera(camera);
}

void SandboxGame::on_fixed_update(rivet::core::GameContext& context, float fixed_delta_time) {
    auto* scene = active_sandbox(context);
    if (scene == nullptr) {
        return;
    }

    if (context.has_service<rivet::physics::PhysicsWorld>()) {
        context.service<rivet::physics::PhysicsWorld>().step(scene->world(), fixed_delta_time);
    }
}

void SandboxGame::on_render(rivet::core::GameContext& context, float interpolation_alpha) {
    (void)interpolation_alpha;
    if (!context.has_service<rivet::render::IRenderer>()) {
        return;
    }

    auto& render = context.service<rivet::render::IRenderer>();
    render.clear({.r = 18, .g = 20, .b = 28, .a = 255});

    const auto* scene = active_sandbox(context);
    if (scene == nullptr) {
        return;
    }

    const auto static_view = scene->world().registry().view<
        rivet::ecs::components::Transform,
        rivet::ecs::components::Collider>();
    for (const auto entity : static_view) {
        const auto& collider = static_view.get<rivet::ecs::components::Collider>(entity);
        if (!collider.is_static) {
            continue;
        }
        const auto& transform = static_view.get<rivet::ecs::components::Transform>(entity);
        render.draw_filled_rect(
            {
                .x = transform.x + collider.offset_x,
                .y = transform.y + collider.offset_y,
                .width = collider.width,
                .height = collider.height,
            },
            {.r = 70, .g = 78, .b = 96, .a = 255});
    }

    const auto& player_transform =
        scene->world().registry().get<rivet::ecs::components::Transform>(scene->player_entity());

    if (player_texture_ != rivet::resources::kInvalidTexture) {
        render.draw_texture(
            player_texture_,
            {.x = player_transform.x, .y = player_transform.y, .width = kSpriteSize, .height = kSpriteSize});
    } else {
        render.draw_filled_rect(
            {.x = player_transform.x, .y = player_transform.y, .width = kSpriteSize, .height = kSpriteSize},
            {.r = 220, .g = 90, .b = 70, .a = 255});
    }
}

} // namespace rivet::game
