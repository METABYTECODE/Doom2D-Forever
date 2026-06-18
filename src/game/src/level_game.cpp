#include <rivet/game/level_game.hpp>
#include <rivet/game/level_scene.hpp>

#include <algorithm>
#include <cmath>

#include <rivet/ecs/components/collider.hpp>
#include <rivet/ecs/components/transform.hpp>
#include <rivet/game/content/content_paths.hpp>
#include <rivet/game/level/level_loader.hpp>
#include <rivet/input/keys.hpp>
#include <rivet/physics/aabb.hpp>

namespace rivet::game {

namespace {

constexpr float kMoveSpeed = 280.0f;
constexpr float kGravity = 1100.0f;
constexpr float kJumpSpeed = 380.0f;
constexpr float kMaxFallSpeed = 640.0f;
constexpr float kGroundTolerance = 6.0f;
constexpr float kJumpBufferTime = 0.12f;
constexpr float kCoyoteTime = 0.1f;
constexpr float kDefaultZoom = 2.0f;

[[nodiscard]] float snap_world(const float value) {
    return std::round(value * kDefaultZoom) / kDefaultZoom;
}

[[nodiscard]] bool is_grounded(
    rivet::ecs::World& world,
    const rivet::ecs::Entity player,
    const rivet::ecs::components::Transform& transform,
    const rivet::ecs::components::Collider& collider) {
    const rivet::physics::Aabb player_box = rivet::physics::make_aabb(transform, collider);
    const float feet_y = transform.y + collider.height;
    constexpr float kSideInset = 2.0f;
    const float probe_left = player_box.x + kSideInset;
    const float probe_right = player_box.x + player_box.width - kSideInset;

    const auto static_view =
        world.registry().view<rivet::ecs::components::Transform, rivet::ecs::components::Collider>();
    for (const auto entity : static_view) {
        if (entity == player) {
            continue;
        }
        const auto& other_collider = static_view.get<rivet::ecs::components::Collider>(entity);
        if (!other_collider.is_static) {
            continue;
        }
        const auto& other_transform = static_view.get<rivet::ecs::components::Transform>(entity);
        const rivet::physics::Aabb other_box = rivet::physics::make_aabb(other_transform, other_collider);

        if (probe_right <= other_box.x || probe_left >= other_box.x + other_box.width) {
            continue;
        }

        const float gap = other_box.y - feet_y;
        if (gap >= -1.5f && gap <= kGroundTolerance) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] LevelScene* active_level(rivet::core::GameContext& context) {
    return static_cast<LevelScene*>(context.scenes().active());
}

[[nodiscard]] rivet::render::Color fallback_tile_color(const std::string& tileset_id, const int tile_id) {
    const std::size_t hash = std::hash<std::string>{}(tileset_id) ^ static_cast<std::size_t>(tile_id * 131);
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

    if (const auto assets = content::resolve_assets_root()) {
        assets_root_ = *assets;
    }

    if (context.has_service<rivet::resources::ResourceManager>()) {
        auto& resources = context.service<rivet::resources::ResourceManager>();
        if (!assets_root_.empty()) {
            tilesets_ = std::make_unique<tileset::TilesetCatalog>(
                resources,
                content::tilesets_dir(assets_root_));
        }
        player_texture_ = resources.create_checkerboard(64, 8, "player");
    }

    if (context.has_service<rivet::render::IRenderer>()) {
        auto& renderer = context.service<rivet::render::IRenderer>();
        rivet::render::Camera2D camera;
        camera.set_viewport(800, 600);
        camera.zoom = kDefaultZoom;
        if (const auto* scene = active_level(context)) {
            camera.center_on(scene->world_width() * 0.5f, scene->world_height() * 0.5f);
        }
        renderer.set_camera(camera);
    }

    if (auto* scene = active_level(context)) {
        sync_player_motion(*scene);
    }
}

void LevelGame::on_detach(rivet::core::GameContext& context) {
    (void)context;
    tilesets_.reset();
    player_texture_ = rivet::resources::kInvalidTexture;
    physics_.clear();
    player_grounded_ = false;
    jump_buffer_time_ = 0.0f;
    coyote_time_ = 0.0f;
    player_motion_ = {};
}

void LevelGame::sync_player_motion(const LevelScene& scene) {
    if (scene.player_entity() == rivet::ecs::kNullEntity) {
        return;
    }

    const auto& transform = scene.world().registry().get<rivet::ecs::components::Transform>(
        scene.player_entity());
    player_motion_.prev_x = transform.x;
    player_motion_.prev_y = transform.y;
    player_motion_.curr_x = transform.x;
    player_motion_.curr_y = transform.y;
}

void LevelGame::update_camera(
    rivet::core::GameContext& context,
    const float player_center_x,
    const float player_center_y) const {
    if (!context.has_service<rivet::render::IRenderer>()) {
        return;
    }

    auto& renderer = context.service<rivet::render::IRenderer>();
    auto camera = renderer.camera();
    camera.center_on(snap_world(player_center_x), snap_world(player_center_y));
    renderer.set_camera(camera);
}

void LevelGame::on_update(rivet::core::GameContext& context, float delta_time) {
    if (context.input().is_key_pressed(rivet::input::Key::Escape)) {
        context.request_quit();
    }

    if (context.input().is_key_pressed(rivet::input::Key::Space)) {
        jump_buffer_time_ = kJumpBufferTime;
    }
    jump_buffer_time_ = std::max(0.0f, jump_buffer_time_ - delta_time);
}

void LevelGame::on_fixed_update(rivet::core::GameContext& context, float fixed_delta_time) {
    auto* scene = active_level(context);
    if (scene == nullptr) {
        return;
    }

    if (scene->player_entity() != rivet::ecs::kNullEntity) {
        auto& registry = scene->world().registry();
        auto& velocity = registry.get<rivet::ecs::components::Velocity>(scene->player_entity());

        velocity.x = context.input().state().move_x * kMoveSpeed;
        velocity.y += kGravity * fixed_delta_time;
        velocity.y = std::min(velocity.y, kMaxFallSpeed);

        const bool can_jump = player_grounded_ || coyote_time_ > 0.0f;
        if (can_jump && jump_buffer_time_ > 0.0f) {
            velocity.y = -kJumpSpeed;
            jump_buffer_time_ = 0.0f;
            coyote_time_ = 0.0f;
        }
    }

    physics_.step(scene->world(), fixed_delta_time);

    if (scene->player_entity() != rivet::ecs::kNullEntity) {
        auto& registry = scene->world().registry();
        auto& transform = registry.get<rivet::ecs::components::Transform>(scene->player_entity());
        auto& velocity = registry.get<rivet::ecs::components::Velocity>(scene->player_entity());
        const auto& collider = registry.get<rivet::ecs::components::Collider>(scene->player_entity());

        const bool grounded = is_grounded(scene->world(), scene->player_entity(), transform, collider);
        if (grounded && velocity.y > 0.0f) {
            velocity.y = 0.0f;
        }

        if (grounded) {
            coyote_time_ = kCoyoteTime;
        } else {
            coyote_time_ = std::max(0.0f, coyote_time_ - fixed_delta_time);
        }
        player_grounded_ = grounded;

        player_motion_.prev_x = player_motion_.curr_x;
        player_motion_.prev_y = player_motion_.curr_y;
        player_motion_.curr_x = transform.x;
        player_motion_.curr_y = transform.y;
    }
}

void LevelGame::draw_level_tiles(rivet::render::IRenderer& renderer, const level::LevelData& data) {
    const float cell_size = static_cast<float>(data.grid_size);

    for (const auto& tile : data.tiles) {
        const tileset::TilesetDef* def = tilesets_ ? tilesets_->find(tile.tileset) : nullptr;
        const int footprint_w = def ? std::max(1, def->tile_width / data.grid_size) : 1;
        const int footprint_h = def ? std::max(1, def->tile_height / data.grid_size) : 1;

        const rivet::render::Rect dest{
            .x = static_cast<float>(tile.x) * cell_size,
            .y = static_cast<float>(tile.y) * cell_size,
            .width = static_cast<float>(footprint_w) * cell_size,
            .height = static_cast<float>(footprint_h) * cell_size,
        };

        if (def != nullptr && def->texture != rivet::resources::kInvalidTexture) {
            renderer.draw_texture(
                def->texture,
                dest,
                tileset::tile_source_rect(*def, tile.id));
            continue;
        }

        renderer.draw_filled_rect(dest, fallback_tile_color(tile.tileset, tile.id));
    }
}

void LevelGame::on_render(rivet::core::GameContext& context, float interpolation_alpha) {
    if (!context.has_service<rivet::render::IRenderer>()) {
        return;
    }

    auto& renderer = context.service<rivet::render::IRenderer>();
    renderer.clear({.r = 18, .g = 20, .b = 28, .a = 255});

    const auto* scene = active_level(context);
    if (scene == nullptr) {
        return;
    }

    const auto& data = scene->data();

    if (scene->player_entity() != rivet::ecs::kNullEntity) {
        const auto& collider =
            scene->world().registry().get<rivet::ecs::components::Collider>(scene->player_entity());

        const float render_x = snap_world(
            player_motion_.prev_x + (player_motion_.curr_x - player_motion_.prev_x) * interpolation_alpha);
        const float render_y = snap_world(
            player_motion_.prev_y + (player_motion_.curr_y - player_motion_.prev_y) * interpolation_alpha);

        update_camera(
            context,
            render_x + collider.width * 0.5f,
            render_y + collider.height * 0.5f);

        draw_level_tiles(renderer, data);

        if (player_texture_ != rivet::resources::kInvalidTexture) {
            renderer.draw_texture(
                player_texture_,
                {.x = render_x, .y = render_y, .width = collider.width, .height = collider.height});
        } else {
            renderer.draw_filled_rect(
                {.x = render_x, .y = render_y, .width = collider.width, .height = collider.height},
                {.r = 220, .g = 90, .b = 70, .a = 255});
        }
        return;
    }

    draw_level_tiles(renderer, data);
}

} // namespace rivet::game
