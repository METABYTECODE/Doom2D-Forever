#include <rivet/game/level_game.hpp>
#include <rivet/game/level_scene.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

#include <rivet/ecs/components/collider.hpp>
#include <rivet/ecs/components/physics_contacts.hpp>
#include <rivet/ecs/components/transform.hpp>
#include <rivet/game/resources/resource_pack.hpp>
#include <spdlog/spdlog.h>
#include <rivet/game/level/level_loader.hpp>
#include <rivet/audio/audio_system.hpp>
#include <rivet/game/model/model_loader.hpp>
#include <rivet/input/keys.hpp>
#include <rivet/physics/aabb.hpp>
#include <rivet/physics/fluid_grid.hpp>
#include <rivet/physics/physics_world.hpp>
#include <rivet/physics/tile_collider.hpp>
#include <rivet/game/tileset/tileset_catalog.hpp>

namespace rivet::game {

namespace {

constexpr float kMoveSpeed = 288.0f;
constexpr float kMoveAccel = 1800.0f;
constexpr float kMoveDecel = 2400.0f;
constexpr float kGravity = 648.0f;
constexpr float kJumpSpeed = 360.0f;
constexpr float kMaxFallSpeed = 1080.0f;
constexpr float kJumpBufferTime = 0.12f;
constexpr float kCoyoteTime = 0.1f;
constexpr float kWaterGravity = 72.0f;
constexpr float kWaterMaxFallSpeed = 120.0f;
constexpr float kSwimSpeed = 220.0f;
constexpr float kWaterDrag = 14.0f;
constexpr float kDefaultZoom = 2.0f;

[[nodiscard]] float snap_world(const float value) {
    return std::round(value * kDefaultZoom) / kDefaultZoom;
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

void apply_horizontal_accel(
    float& velocity_x,
    const float target_x,
    const float fixed_delta_time) {
    if (velocity_x < target_x) {
        velocity_x = std::min(target_x, velocity_x + kMoveAccel * fixed_delta_time);
    } else if (velocity_x > target_x) {
        velocity_x = std::max(target_x, velocity_x - kMoveDecel * fixed_delta_time);
    }
}

void apply_drag(float& velocity, const float drag, const float fixed_delta_time) {
    if (std::abs(velocity) <= 1.0f) {
        velocity = 0.0f;
        return;
    }

    if (velocity > 0.0f) {
        velocity = std::max(0.0f, velocity - drag * fixed_delta_time);
    } else {
        velocity = std::min(0.0f, velocity + drag * fixed_delta_time);
    }
}

} // namespace

LevelGame::LevelGame(std::filesystem::path level_path)
    : level_path_(std::move(level_path)) {}

void LevelGame::on_attach(rivet::core::GameContext& context) {
    level_ = level::load_level(level_path_);
    fluids_ = rivet::physics::FluidGrid::from_grid(
        level_.fluids,
        static_cast<float>(level_.grid_size));

    if (const auto assets = resources::resolve_assets_root()) {
        assets_root_ = *assets;
        resource_pack_ = resources::ResourcePack::load(assets_root_, level_.resource_pack);
    }

    level::LevelSpawnContext spawn_context;
    if (resource_pack_.has_value()) {
        spawn_context.pack = &*resource_pack_;
    }

    context.scenes().push(std::make_unique<LevelScene>(level_, spawn_context));

    if (context.has_service<rivet::physics::PhysicsWorld>()) {
        auto& physics = context.service<rivet::physics::PhysicsWorld>();
        physics.clear();
        physics.set_tile_collider(rivet::physics::TileCollider::from_grid(
            level_.collision,
            static_cast<float>(level_.grid_size)));
        physics.set_broadphase_cell_size(static_cast<float>(level_.grid_size));

        if (auto* scene = active_level(context)) {
            physics.set_world_bounds({
                .min_x = 0.0f,
                .min_y = 0.0f,
                .max_x = scene->world_width(),
                .max_y = scene->world_height(),
                .enabled = true,
            });
        }
    }

    if (context.has_service<rivet::resources::ResourceManager>()) {
        auto& resources = context.service<rivet::resources::ResourceManager>();
        if (resource_pack_) {
            tilesets_ = std::make_unique<tileset::TilesetCatalog>(
                resources,
                resource_pack_->tilesets_dir());

            if (!level_.background.empty()) {
                if (const auto bg_path = resource_pack_->resolve_background(level_.background)) {
                    background_texture_ = resources.load_texture(*bg_path);
                } else {
                    spdlog::warn("Background asset not found: {}", level_.background);
                }
            }

            if (!level_.music.empty()) {
                if (const auto resolved_music = resource_pack_->resolve_music(level_.music)) {
                    music_path_ = *resolved_music;
                    spdlog::info("Level music: {} ({})", level_.music, music_path_.string());
                } else {
                    spdlog::warn("Music asset not found: {}", level_.music);
                }
            }

            if (const auto jump_sfx = resource_pack_->resolve_sfx("jump")) {
                jump_sfx_path_ = *jump_sfx;
            }

            if (const auto model_path = resource_pack_->resolve_model("player")) {
                if (const auto loaded = model::load_model(*model_path, resources, *resource_pack_)) {
                    player_model_ = std::move(*loaded);
                    player_animator_.set_model(&*player_model_);
                    spdlog::info(
                        "Loaded player model: {} ({} animations, collider {:.0f}x{:.0f})",
                        player_model_->id,
                        player_model_->animations.size(),
                        player_model_->collider.width,
                        player_model_->collider.height);
                } else {
                    spdlog::warn("Failed to load player model from {}", model_path->string());
                }
            } else {
                spdlog::warn("Player model not found in resource pack");
            }
        }
    }

    if (!music_path_.empty() && context.has_service<rivet::audio::AudioSystem>()) {
        context.service<rivet::audio::AudioSystem>().play_music(music_path_);
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
    if (context.has_service<rivet::audio::AudioSystem>()) {
        context.service<rivet::audio::AudioSystem>().stop_music();
    }
    if (context.has_service<rivet::physics::PhysicsWorld>()) {
        context.service<rivet::physics::PhysicsWorld>().clear();
    }
    tilesets_.reset();
    background_texture_ = rivet::resources::kInvalidTexture;
    resource_pack_.reset();
    music_path_.clear();
    jump_sfx_path_.clear();
    fluids_ = {};
    player_model_.reset();
    player_animator_.set_model(nullptr);
    player_velocity_x_ = 0.0f;
    player_velocity_y_ = 0.0f;
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

void LevelGame::update_patrols(
    rivet::ecs::World& world,
    const std::vector<rivet::ecs::Entity>& patrols) {
    for (const auto entity : patrols) {
        if (!world.registry().all_of<
                rivet::ecs::components::Velocity,
                rivet::ecs::components::PhysicsContacts>(entity)) {
            continue;
        }
        auto& velocity = world.registry().get<rivet::ecs::components::Velocity>(entity);
        const auto& contacts = world.registry().get<rivet::ecs::components::PhysicsContacts>(entity);
        if (contacts.wall_left || contacts.wall_right) {
            velocity.x = -velocity.x;
        }
    }
}

void LevelGame::on_update(rivet::core::GameContext& context, float delta_time) {
    if (context.input().is_key_pressed(rivet::input::Key::Escape)) {
        context.request_quit();
    }

    animation_time_ += delta_time;

    if (context.input().is_key_pressed(rivet::input::Key::Space)) {
        jump_buffer_time_ = kJumpBufferTime;
    }
    jump_buffer_time_ = std::max(0.0f, jump_buffer_time_ - delta_time);

    if (auto* scene = active_level(context)) {
        if (scene->player_entity() != rivet::ecs::kNullEntity) {
            const auto& velocity =
                scene->world().registry().get<rivet::ecs::components::Velocity>(scene->player_entity());
            update_player_animation(delta_time, velocity.x, velocity.y);
        }
    }
}

void LevelGame::on_fixed_update(rivet::core::GameContext& context, float fixed_delta_time) {
    auto* scene = active_level(context);
    if (scene == nullptr) {
        return;
    }

    update_patrols(scene->world(), scene->patrol_entities());

    if (scene->player_entity() != rivet::ecs::kNullEntity) {
        auto& registry = scene->world().registry();

        if (registry.all_of<rivet::ecs::components::PhysicsContacts>(scene->player_entity())) {
            player_grounded_ =
                registry.get<rivet::ecs::components::PhysicsContacts>(scene->player_entity()).floor;
        } else {
            player_grounded_ = false;
        }
        if (player_grounded_) {
            coyote_time_ = kCoyoteTime;
        } else {
            coyote_time_ = std::max(0.0f, coyote_time_ - fixed_delta_time);
        }

        auto& velocity = registry.get<rivet::ecs::components::Velocity>(scene->player_entity());
        player_velocity_x_ = velocity.x;
        player_velocity_y_ = velocity.y;
        const auto& transform = registry.get<rivet::ecs::components::Transform>(scene->player_entity());
        const auto& collider = registry.get<rivet::ecs::components::Collider>(scene->player_entity());

        const auto fluid_sample = fluids_.sample_aabb(rivet::physics::make_aabb(transform, collider));
        const bool in_fluid = fluid_sample.immersed && fluid_sample.id != 0;
        const bool swim_up = context.input().is_key_down(rivet::input::Key::Space);

        const float target_vx = context.input().state().move_x * (in_fluid ? kSwimSpeed : kMoveSpeed);
        apply_horizontal_accel(velocity.x, target_vx, fixed_delta_time);

        if (in_fluid) {
            if (swim_up) {
                velocity.y = -kSwimSpeed;
            } else {
                velocity.y += kWaterGravity * fixed_delta_time;
                velocity.y = std::min(velocity.y, kWaterMaxFallSpeed);
            }
            apply_drag(velocity.x, kWaterDrag, fixed_delta_time);
            apply_drag(velocity.y, kWaterDrag, fixed_delta_time);
        } else {
            velocity.y += kGravity * fixed_delta_time;
            velocity.y = std::min(velocity.y, kMaxFallSpeed);

            const bool can_jump = player_grounded_ || coyote_time_ > 0.0f;
            if (can_jump && jump_buffer_time_ > 0.0f) {
                velocity.y = -kJumpSpeed;
                jump_buffer_time_ = 0.0f;
                coyote_time_ = 0.0f;
                if (!jump_sfx_path_.empty() && context.has_service<rivet::audio::AudioSystem>()) {
                    context.service<rivet::audio::AudioSystem>().play_sfx(jump_sfx_path_);
                }
            }
        }
    }

    if (context.has_service<rivet::physics::PhysicsWorld>()) {
        context.service<rivet::physics::PhysicsWorld>().step(scene->world(), fixed_delta_time);
    }

    if (scene->player_entity() != rivet::ecs::kNullEntity) {
        const auto& registry = scene->world().registry();
        const auto& transform = registry.get<rivet::ecs::components::Transform>(scene->player_entity());
        player_motion_.prev_x = player_motion_.curr_x;
        player_motion_.prev_y = player_motion_.curr_y;
        player_motion_.curr_x = transform.x;
        player_motion_.curr_y = transform.y;
    }
}

void LevelGame::draw_level_background(
    rivet::render::IRenderer& renderer,
    const LevelScene& scene) const {
    if (background_texture_ == rivet::resources::kInvalidTexture) {
        return;
    }

    renderer.draw_texture(
        background_texture_,
        {
            .x = 0.0f,
            .y = 0.0f,
            .width = scene.world_width(),
            .height = scene.world_height(),
        });
}

void LevelGame::draw_level_world(
    rivet::render::IRenderer& renderer,
    const level::LevelData& data,
    const float animation_time,
    const float player_pivot_x,
    const float player_pivot_y,
    const float player_collider_offset_x,
    const float player_collider_offset_y,
    const float player_collider_width,
    const float player_collider_height,
    const int player_z,
    const bool draw_player) {
    struct DrawItem {
        int z = 0;
        float sort_y = 0.0f;
        rivet::resources::TextureHandle texture = rivet::resources::kInvalidTexture;
        rivet::render::Rect dest{};
        rivet::render::Rect source{};
        rivet::render::Color fallback{};
        bool use_fallback = false;
    };

    std::vector<DrawItem> items;
    const float cell_size = static_cast<float>(data.grid_size);

    for (const auto& layer : data.tile_layers) {
        for (const auto& tile : layer.tiles) {
            const auto frame = level::tile_frame_at_time(tile, animation_time);
            const tileset::TilesetDef* def = tilesets_ ? tilesets_->find(frame.tileset) : nullptr;

            DrawItem item;
            item.z = layer.z;
            item.dest =
                def != nullptr
                    ? tileset::tile_dest_rect(*def, static_cast<float>(tile.x), static_cast<float>(tile.y))
                    : rivet::render::Rect{
                          .x = static_cast<float>(tile.x),
                          .y = static_cast<float>(tile.y),
                          .width = cell_size,
                          .height = cell_size,
                      };
            item.sort_y = item.dest.y + item.dest.height;

            if (def != nullptr && def->texture != rivet::resources::kInvalidTexture) {
                item.texture = def->texture;
                item.source = tileset::tile_source_rect(*def, frame.id);
            } else {
                item.use_fallback = true;
                item.fallback = fallback_tile_color(frame.tileset, frame.id);
            }
            items.push_back(item);
        }
    }

    if (draw_player) {
        DrawItem player_item;
        player_item.z = player_z;
        if (player_model_.has_value() && player_animator_.has_clip()) {
            const auto frame = player_animator_.current_frame();
            if (frame.texture != rivet::resources::kInvalidTexture) {
                player_item.texture = frame.texture;
                player_item.source = frame.source;
                player_item.dest = model::model_sprite_dest_rect(
                    player_pivot_x,
                    player_pivot_y,
                    player_model_->pivot,
                    frame.frame_width,
                    frame.frame_height);
            } else {
                player_item.use_fallback = true;
                player_item.fallback = {.r = 220, .g = 90, .b = 70, .a = 255};
                player_item.dest = {
                    .x = player_pivot_x + player_collider_offset_x,
                    .y = player_pivot_y + player_collider_offset_y,
                    .width = player_collider_width,
                    .height = player_collider_height,
                };
            }
        } else {
            player_item.use_fallback = true;
            player_item.fallback = {.r = 220, .g = 90, .b = 70, .a = 255};
            player_item.dest = {
                .x = player_pivot_x + player_collider_offset_x,
                .y = player_pivot_y + player_collider_offset_y,
                .width = player_collider_width,
                .height = player_collider_height,
            };
        }
        player_item.sort_y = player_item.dest.y + player_item.dest.height;
        items.push_back(player_item);
    }

    std::stable_sort(
        items.begin(),
        items.end(),
        [](const DrawItem& a, const DrawItem& b) {
            if (a.z != b.z) {
                return a.z < b.z;
            }
            return a.sort_y < b.sort_y;
        });

    for (const auto& item : items) {
        if (!item.use_fallback && item.texture != rivet::resources::kInvalidTexture) {
            renderer.draw_texture(item.texture, item.dest, item.source);
        } else if (item.use_fallback) {
            renderer.draw_filled_rect(item.dest, item.fallback);
        }
    }
}

int LevelGame::player_render_z() const {
    for (const auto& object : level_.objects) {
        if (object.type == "player") {
            return object.z;
        }
    }
    return 0;
}

void LevelGame::update_player_animation(
    const float delta_time,
    const float velocity_x,
    const float velocity_y) {
    if (!player_model_.has_value()) {
        return;
    }

    const std::string desired =
        model::select_player_locomotion_animation(player_grounded_, velocity_x, velocity_y);
    player_animator_.set_animation(desired, false);
    player_animator_.update(delta_time);
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
            render_x + collider.offset_x + collider.width * 0.5f,
            render_y + collider.offset_y + collider.height * 0.5f);

        draw_level_background(renderer, *scene);
        draw_level_world(
            renderer,
            data,
            animation_time_,
            render_x,
            render_y,
            collider.offset_x,
            collider.offset_y,
            collider.width,
            collider.height,
            player_render_z(),
            true);
        return;
    }

    draw_level_background(renderer, *scene);
    draw_level_world(
        renderer,
        data,
        animation_time_,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0,
        false);
}

} // namespace rivet::game
