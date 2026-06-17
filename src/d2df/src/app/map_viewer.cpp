#include <d2df/app/map_viewer.hpp>

#if D2DF_DEBUG_UI
#include <d2df/debug/debug_ui.hpp>
#include <d2df/debug/debug_world_view.hpp>
#endif

#include <algorithm>

#include <SDL.h>
#include <spdlog/spdlog.h>

#include <cmath>
#include <algorithm>

#include <d2df/core/event_bus.hpp>
#include <d2df/core/game_events.hpp>
#include <d2df/map/map_catalog.hpp>
#include <d2df/core/types.hpp>
#include <d2df/map/item_types.hpp>
#include <d2df/map/map_json_loader.hpp>
#include <d2df/map/monster_types.hpp>
#include <d2df/sim/item_system.hpp>
#include <d2df/sim/effect_types.hpp>
#include <d2df/sim/projectile_system.hpp>
#include <d2df/sim/projectile_types.hpp>
#include <d2df/sim/player_types.hpp>
#include <d2df/sim/weapon_types.hpp>

namespace d2df {
namespace {

std::filesystem::path normalize_map_path(const std::filesystem::path& path) {
    std::error_code ec;
    const auto absolute = std::filesystem::absolute(path, ec);
    if (ec) {
        return path.lexically_normal();
    }
    const auto canonical = std::filesystem::weakly_canonical(absolute, ec);
    return ec ? absolute.lexically_normal() : canonical;
}

std::size_t find_map_index(const std::vector<std::filesystem::path>& map_list,
                           const std::filesystem::path& map_path) {
    const auto target = normalize_map_path(map_path);
    for (std::size_t i = 0; i < map_list.size(); ++i) {
        if (normalize_map_path(map_list[i]) == target) {
            return i;
        }
    }
    return 0;
}

} // namespace

void set_monster_draw_color(map::MonsterType type, SDL_Renderer* renderer) {
    switch (type) {
    case map::MonsterType::Zomby:
        SDL_SetRenderDrawColor(renderer, 120, 180, 80, 220);
        break;
    case map::MonsterType::Imp:
        SDL_SetRenderDrawColor(renderer, 200, 120, 60, 220);
        break;
    case map::MonsterType::Demon:
        SDL_SetRenderDrawColor(renderer, 220, 60, 60, 220);
        break;
    case map::MonsterType::Serg:
        SDL_SetRenderDrawColor(renderer, 160, 160, 80, 220);
        break;
    case map::MonsterType::Soul:
        SDL_SetRenderDrawColor(renderer, 255, 140, 40, 220);
        break;
    case map::MonsterType::Pain:
        SDL_SetRenderDrawColor(renderer, 180, 60, 200, 220);
        break;
    case map::MonsterType::Caco:
        SDL_SetRenderDrawColor(renderer, 220, 60, 120, 220);
        break;
    case map::MonsterType::Cgun:
        SDL_SetRenderDrawColor(renderer, 140, 140, 160, 220);
        break;
    case map::MonsterType::Barrel:
        SDL_SetRenderDrawColor(renderer, 160, 100, 40, 220);
        break;
    case map::MonsterType::Fish:
        SDL_SetRenderDrawColor(renderer, 80, 160, 220, 220);
        break;
    default:
        SDL_SetRenderDrawColor(renderer, 180, 60, 200, 220);
        break;
    }
}

MapViewer::MapViewer(SDL_Renderer* renderer, std::filesystem::path content_root,
                     std::filesystem::path map_path, EventBus* events)
    : renderer_(renderer)
    , content_root_(std::move(content_root))
    , events_(events) {
    assets_.load(content_root_);
    text_.init(content_root_);
    sound_.set_content_root(content_root_);
    sound_.bind_assets(assets_);
    if (sound_.init()) {
        if (events_ != nullptr) {
            events_->subscribe<events::ItemPickedUp>([this](const events::ItemPickedUp& event) {
                const auto item_type = static_cast<map::ItemType>(event.item_type);
                if (const char* sfx = map::item_pickup_sfx(item_type)) {
                    sound_.play(sfx);
                }
            });
            events_->subscribe<events::ItemRespawned>(
                [this](const events::ItemRespawned&) { sound_.play("sfx.world.respawnitem"); });
            events_->subscribe<events::PlayerTeleported>(
                [this](const events::PlayerTeleported& event) {
                    sound_.play(event.blocked ? "sfx.world.noteleport" : "sfx.world.teleport");
                });
            events_->subscribe<events::PlayerFired>([this](const events::PlayerFired& event) {
                if (event.shooter_id != kPlayerEntityId) {
                    return;
                }
                const auto weapon = static_cast<sim::WeaponId>(event.weapon);
                if (const char* sfx = sim::weapon_fire_sfx(weapon)) {
                    sound_.play(sfx);
                }
            });
            events_->subscribe<events::MonsterDied>([this](const events::MonsterDied& event) {
                const auto type = static_cast<map::MonsterType>(event.monster_type);
                if (const char* sfx = map::monster_die_sfx(type)) {
                    sound_.play(sfx);
                }
                if (type == map::MonsterType::Barrel) {
                    sound_.play("sfx.world.exploderocket");
                }
            });
            events_->subscribe<events::MonsterAlerted>([this](const events::MonsterAlerted& event) {
                const auto type = static_cast<map::MonsterType>(event.monster_type);
                if (const char* sfx = map::monster_alert_sfx(type, event.entity_id)) {
                    sound_.play(sfx);
                }
            });
            events_->subscribe<events::MonsterFired>([this](const events::MonsterFired& event) {
                const auto type = static_cast<map::MonsterType>(event.monster_type);
                if (const char* sfx = map::monster_attack_sfx(type)) {
                    sound_.play(sfx);
                }
            });
            events_->subscribe<events::WorldDoorChanged>(
                [this](const events::WorldDoorChanged& event) {
                    switch (event.sound) {
                    case events::DoorSound::Open:
                        sound_.play("sfx.world.dooropen");
                        break;
                    case events::DoorSound::Close:
                        sound_.play("sfx.world.doorclose");
                        break;
                    default:
                        break;
                    }
                });
            events_->subscribe<events::WorldSwitchActivated>(
                [this](const events::WorldSwitchActivated&) {
                    sound_.play("sfx.world.switch0");
                });
            events_->subscribe<events::WorldExplosion>([this](const events::WorldExplosion& event) {
                if (const char* sfx = sim::explosion_sfx(event.kind)) {
                    sound_.play(sfx);
                }
                spawn_explosion_effect(event);
            });
            events_->subscribe<events::WorldSmokePuff>(
                [this](const events::WorldSmokePuff& event) { spawn_smoke_effect(event); });
            events_->subscribe<events::WorldBubblePuff>(
                [this](const events::WorldBubblePuff& event) { spawn_bubble_effect(event); });
            events_->subscribe<events::PlayerDamaged>([this](const events::PlayerDamaged& event) {
                if (event.amount <= 0) {
                    return;
                }
                if (event.source == events::DamageSource::Melee) {
                    sound_.play("sfx.world.hitpunch");
                } else {
                    sound_.play("sfx.world.hit");
                }
            });
        }
    }
    map_list_ = map::list_map_json_files(content_root_);
    spdlog::info("Map catalog: {} JSON maps", map_list_.size());
    load_map(std::move(map_path));
}

void MapViewer::load_map(const std::filesystem::path& map_path) {
    map_ = map::load_map_json_v1(map_path);
    textures_ = std::make_unique<render::TextureCache>(renderer_, assets_);
    triggers_.reset(map_);
    collision_.build_from_panels(triggers_.panels());
    world_.reset_player(map_);
    camera_.center_x = world_.player().x + world_.player().width * 0.5f;
    camera_.center_y = world_.player().y + world_.player().height * 0.5f;
    camera_.scale = 2;
    free_camera_ = false;

    map_index_ = find_map_index(map_list_, map_path);

    effects_.clear();

    spdlog::info("Map [{}/{}]: {} ({}x{}, {} panels, {} areas)", map_index_ + 1,
                 map_list_.size(), map_.name, map_.size.width, map_.size.height, map_.panels.size(),
                 map_.areas.size());

    if (sound_.enabled() && !map_.music.empty()) {
        sound_.play_music(map_.music.c_str());
    }
}

void MapViewer::cycle_map(int direction) {
    if (map_list_.size() <= 1) {
        spdlog::warn("Map cycle unavailable ({} maps in catalog)", map_list_.size());
        return;
    }

    const std::size_t start_index = map_index_;
    do {
        if (direction > 0) {
            map_index_ = (map_index_ + 1) % map_list_.size();
        } else {
            map_index_ = (map_index_ + map_list_.size() - 1) % map_list_.size();
        }

        try {
            load_map(map_list_[map_index_]);
            return;
        } catch (const std::exception& ex) {
            spdlog::warn("Skip broken map {}: {}", map_list_[map_index_].filename().string(),
                         ex.what());
        }
    } while (map_index_ != start_index);
}

void MapViewer::handle_key_down(int sym, SDL_Scancode scancode) {
    switch (scancode) {
    case SDL_SCANCODE_LEFTBRACKET:
    case SDL_SCANCODE_PAGEUP:
        cycle_map(-1);
        return;
    case SDL_SCANCODE_RIGHTBRACKET:
    case SDL_SCANCODE_PAGEDOWN:
        cycle_map(1);
        return;
    case SDL_SCANCODE_1:
        weapon_select_request_ = static_cast<int>(sim::WeaponId::Knuckles);
        return;
    case SDL_SCANCODE_2:
        weapon_select_request_ = static_cast<int>(sim::WeaponId::Pistol);
        return;
    case SDL_SCANCODE_3:
        weapon_select_request_ = static_cast<int>(sim::WeaponId::Chaingun);
        return;
    case SDL_SCANCODE_4:
        weapon_select_request_ = static_cast<int>(sim::WeaponId::Shotgun1);
        return;
    case SDL_SCANCODE_5:
        weapon_select_request_ = static_cast<int>(sim::WeaponId::RocketLauncher);
        return;
    case SDL_SCANCODE_6:
        weapon_select_request_ = static_cast<int>(sim::WeaponId::Plasma);
        return;
    case SDL_SCANCODE_7:
        weapon_select_request_ = static_cast<int>(sim::WeaponId::Saw);
        return;
    case SDL_SCANCODE_8:
        weapon_select_request_ = static_cast<int>(sim::WeaponId::Shotgun2);
        return;
    case SDL_SCANCODE_9:
        weapon_select_request_ = static_cast<int>(sim::WeaponId::Flamethrower);
        return;
    case SDL_SCANCODE_0:
        weapon_select_request_ = static_cast<int>(sim::WeaponId::Bfg);
        return;
    case SDL_SCANCODE_MINUS:
        weapon_select_request_ = static_cast<int>(sim::WeaponId::SuperChaingun);
        return;
    default:
        break;
    }

    switch (sym) {
    case SDLK_EQUALS:
    case SDLK_PLUS:
    case SDLK_KP_PLUS:
        camera_.zoom_in();
        break;
    case SDLK_KP_MINUS:
        camera_.zoom_out();
        break;
    case SDLK_LEFTBRACKET:
        cycle_map(-1);
        break;
    case SDLK_RIGHTBRACKET:
        cycle_map(1);
        break;
    case SDLK_PAGEUP:
        cycle_map(-1);
        break;
    case SDLK_PAGEDOWN:
        cycle_map(1);
        break;
    case SDLK_c:
        free_camera_ = !free_camera_;
        break;
    case SDLK_a:
    case SDLK_LEFT:
        key_left_ = true;
        break;
    case SDLK_d:
    case SDLK_RIGHT:
        key_right_ = true;
        break;
    case SDLK_SPACE:
    case SDLK_w:
    case SDLK_UP:
        key_jump_ = true;
        key_aim_up_ = true;
        break;
    case SDLK_e:
    case SDLK_f:
        if (!key_use_) {
            key_use_edge_ = true;
        }
        key_use_ = true;
        break;
    case SDLK_q:
        key_weapon_prev_ = true;
        break;
    case SDLK_b:
        weapon_select_request_ = static_cast<int>(sim::WeaponId::Bfg);
        break;
    case SDLK_g:
        weapon_select_request_ = static_cast<int>(sim::WeaponId::SuperChaingun);
        break;
    case SDLK_s:
    case SDLK_DOWN:
        if (!key_jump_) {
            key_aim_down_ = true;
        }
        break;
    default:
        break;
    }
}

void MapViewer::handle_key_up(int sym) {
    switch (sym) {
    case SDLK_a:
    case SDLK_LEFT:
        key_left_ = false;
        break;
    case SDLK_d:
    case SDLK_RIGHT:
        key_right_ = false;
        break;
    case SDLK_SPACE:
    case SDLK_w:
    case SDLK_UP:
        key_jump_ = false;
        key_aim_up_ = false;
        break;
    case SDLK_e:
    case SDLK_f:
        key_use_ = false;
        break;
    case SDLK_q:
        key_weapon_prev_ = false;
        break;
    case SDLK_s:
    case SDLK_DOWN:
        key_aim_down_ = false;
        break;
    default:
        break;
    }
}

void MapViewer::handle_mouse_wheel(int y) {
    if (y > 0) {
        camera_.zoom_in();
    } else if (y < 0) {
        camera_.zoom_out();
    }
}

void MapViewer::fixed_update() {
    if (free_camera_) {
        const Uint8* keys = SDL_GetKeyboardState(nullptr);
        float dx = 0.0f;
        float dy = 0.0f;

        if (keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP]) {
            dy -= 1.0f;
        }
        if (keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN]) {
            dy += 1.0f;
        }
        if (keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT]) {
            dx -= 1.0f;
        }
        if (keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT]) {
            dx += 1.0f;
        }

        if (dx == 0.0f && dy == 0.0f) {
            return;
        }

        const float length = std::sqrt(dx * dx + dy * dy);
        dx /= length;
        dy /= length;

        const float speed = camera_move_speed_ * fixed_timestep_.step_seconds();
        camera_.pan(dx * speed, dy * speed);
        return;
    }

    sim::PlayerInput input{key_left_, key_right_, key_jump_, key_aim_up_, key_aim_down_, false,
                           key_weapon_prev_, false, weapon_select_request_};

    const Uint32 mouse_buttons = SDL_GetMouseState(nullptr, nullptr);
    if ((mouse_buttons & SDL_BUTTON_LMASK) != 0 || (mouse_buttons & SDL_BUTTON_RMASK) != 0) {
        input.fire = true;
    }

    weapon_select_request_ = -1;
    key_weapon_prev_ = false;

    auto& player = world_.player();
    collision_.build_from_panels(triggers_.panels());
    world_.fixed_update(collision_, input, events_, map_.size.width, map_.size.height, &triggers_);
    triggers_.update(player, key_use_edge_, events_, &world_.targets(), &collision_);
    collision_.build_from_panels(triggers_.panels());
    key_use_edge_ = false;

    if (!player.alive()) {
        world_.respawn_player();
    }

    if (triggers_.consume_exit_request()) {
        if (events_ != nullptr) {
            events_->publish(events::MapExitRequested{});
        }
        cycle_map(1);
    }

    tick_effects();
}

void MapViewer::update_camera_follow(float render_alpha) {
    if (free_camera_) {
        return;
    }
    const auto& player = world_.player();
    camera_.center_x = player.render_x(render_alpha) + player.width * 0.5f;
    camera_.center_y = player.render_y(render_alpha) + player.height * 0.5f;
}

void MapViewer::update(float frame_dt) {
    fixed_timestep_.advance(frame_dt);
    while (fixed_timestep_.consume_fixed_step()) {
        fixed_update();
    }
}

void MapViewer::draw_sky(int viewport_w, int viewport_h) {
    if (map_.sky.empty() || textures_ == nullptr) {
        return;
    }

    SDL_Texture* sky = textures_->get(map_.sky);
    if (sky == nullptr) {
        return;
    }

    int tex_w = 0;
    int tex_h = 0;
    SDL_QueryTexture(sky, nullptr, nullptr, &tex_w, &tex_h);
    if (tex_w <= 0 || tex_h <= 0) {
        return;
    }

    const int tile_w = tex_w * camera_.scale;
    const int tile_h = tex_h * camera_.scale;
    const int sky_band = std::max(tile_h, viewport_h / 4);
    const float parallax = 0.35f;
    int offset_x =
        static_cast<int>(-camera_.center_x * parallax * static_cast<float>(camera_.scale)) % tile_w;
    if (offset_x > 0) {
        offset_x -= tile_w;
    }

    for (int y = 0; y < sky_band; y += tile_h) {
        for (int x = offset_x; x < viewport_w; x += tile_w) {
            const SDL_Rect dst{x, y, tile_w, tile_h};
            SDL_RenderCopy(renderer_, sky, nullptr, &dst);
        }
    }
}

namespace {

bool draw_player_layer(SDL_Renderer* renderer, render::TextureCache* textures,
                       const render::Camera2D& camera, int viewport_w, int viewport_h,
                       const char* texture_id, float world_x, float world_y, int frame_width,
                       int frame_height, int frame_count, int frame_index, bool flip_horizontal,
                       Uint8 alpha, bool colorize, std::uint8_t color_r, std::uint8_t color_g,
                       std::uint8_t color_b) {
    if (texture_id == nullptr || textures == nullptr) {
        return false;
    }

    SDL_Texture* texture = textures->get(texture_id);
    if (texture == nullptr) {
        return false;
    }

    int tex_w = 0;
    int tex_h = 0;
    SDL_QueryTexture(texture, nullptr, nullptr, &tex_w, &tex_h);
    if (tex_w <= 0 || tex_h <= 0) {
        return false;
    }

    const int resolved_frame_w = frame_width > 0 ? frame_width : tex_w;
    const int resolved_frame_h = frame_height > 0 ? frame_height : tex_h;
    const int frames_in_texture =
        resolved_frame_w > 0 ? std::max(1, tex_w / resolved_frame_w) : 1;
    const int resolved_frame_count = std::min(frame_count, frames_in_texture);
    const int resolved_frame_index =
        std::clamp(frame_index, 0, std::max(0, resolved_frame_count - 1));

    int dst_x = 0;
    int dst_y = 0;
    int dst_w = 0;
    int dst_h = 0;
    camera.world_rect_to_screen(world_x, world_y, static_cast<float>(resolved_frame_w),
                                static_cast<float>(resolved_frame_h), viewport_w, viewport_h,
                                dst_x, dst_y, dst_w, dst_h);
    if (dst_w <= 0 || dst_h <= 0) {
        return false;
    }

    SDL_SetTextureAlphaMod(texture, alpha);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    if (colorize) {
        SDL_SetTextureColorMod(texture, color_r, color_g, color_b);
    } else {
        SDL_SetTextureColorMod(texture, 255, 255, 255);
    }

    const SDL_Rect src{resolved_frame_index * resolved_frame_w, 0, resolved_frame_w,
                       resolved_frame_h};
    const SDL_Rect dst{dst_x, dst_y, dst_w, dst_h};
    if (flip_horizontal) {
        SDL_RenderCopyEx(renderer, texture, &src, &dst, 0.0, nullptr, SDL_FLIP_HORIZONTAL);
    } else {
        SDL_RenderCopy(renderer, texture, &src, &dst);
    }
    return true;
}

} // namespace

void MapViewer::draw_player(int viewport_w, int viewport_h) {
    if (free_camera_) {
        return;
    }

    const auto& player = world_.player();
    const float alpha = fixed_timestep_.render_alpha();
    const float hitbox_x = player.render_x(alpha);
    const float hitbox_y = player.render_y(alpha);

    const auto& combat = player.combat();
    const bool firing = combat.is_reloading(combat.current_weapon);
    const auto anim = sim::resolve_player_anim(
        player.on_ground(), player.vel_x, key_aim_up_, key_aim_down_, firing,
        combat.current_weapon, player.pain_ticks(), player.death_phase());
    const auto sprite_set = sim::player_sprite_set(anim);
    const auto placement =
        sim::player_sprite_placement(hitbox_x, hitbox_y, combat.facing_left);
    const int frame_index =
        sim::player_anim_frame_index(anim, player.tick(), player.vel_x);
    const Uint8 draw_alpha = player.has_invis() ? 200 : 255;
    const auto team_color = sim::default_player_team_color();

    if (sim::should_draw_weapon(combat.current_weapon, anim)) {
        if (const char* weapon_texture =
                sim::player_weapon_texture_id(combat.current_weapon, anim, firing)) {
            float weapon_x = placement.x;
            float weapon_y = placement.y;
            sim::PlayerWeaponDrawOffset weapon_offset{};
            if (sim::player_weapon_draw_offset(combat.current_weapon, anim, frame_index,
                                               combat.facing_left, weapon_offset)) {
                weapon_x += weapon_offset.dx;
                weapon_y += weapon_offset.dy;
            }

            (void)draw_player_layer(renderer_, textures_.get(), camera_, viewport_w, viewport_h,
                                    weapon_texture, weapon_x, weapon_y, 0, 0, 1, 0,
                                    placement.flip_horizontal, draw_alpha, false, 255, 255, 255);
        }
    }

    const bool drew_body = draw_player_layer(
        renderer_, textures_.get(), camera_, viewport_w, viewport_h, sprite_set.body.texture_id,
        placement.x, placement.y, sprite_set.body.frame_width, sprite_set.body.frame_height,
        sprite_set.body.frame_count, frame_index, placement.flip_horizontal, draw_alpha, false,
        255, 255, 255);

    if (drew_body && sprite_set.mask_texture_id != nullptr) {
        (void)draw_player_layer(renderer_, textures_.get(), camera_, viewport_w, viewport_h,
                                sprite_set.mask_texture_id, placement.x, placement.y,
                                sprite_set.body.frame_width, sprite_set.body.frame_height,
                                sprite_set.body.frame_count, frame_index,
                                placement.flip_horizontal, draw_alpha, true, team_color.r,
                                team_color.g, team_color.b);
    }

    if (drew_body) {
        return;
    }

    int dst_x = 0;
    int dst_y = 0;
    int dst_w = 0;
    int dst_h = 0;
    camera_.world_rect_to_screen(hitbox_x, hitbox_y, player.width, player.height, viewport_w,
                                 viewport_h, dst_x, dst_y, dst_w, dst_h);

    if (player.in_water()) {
        SDL_SetRenderDrawColor(renderer_, 48, 120, 200, 220);
    } else if (player.in_acid()) {
        SDL_SetRenderDrawColor(renderer_, 48, 200, 48, 220);
    } else if (player.on_lift()) {
        SDL_SetRenderDrawColor(renderer_, 200, 160, 48, 220);
    } else {
        SDL_SetRenderDrawColor(renderer_, 64, 175, 48, 220);
    }
    const SDL_Rect rect{dst_x, dst_y, dst_w, dst_h};
    SDL_RenderFillRect(renderer_, &rect);
    SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer_, &rect);
}

void MapViewer::draw_projectiles(int viewport_w, int viewport_h) {
    if (free_camera_) {
        return;
    }

    for (const auto& projectile : world_.projectiles().projectiles()) {
        if (!projectile.active) {
            continue;
        }

        const auto sprite = sim::projectile_sprite(projectile.kind);
        const float alpha = fixed_timestep_.render_alpha();
        const float px = projectile.prev_x + (projectile.x - projectile.prev_x) * alpha +
                         sprite.draw_offset_x;
        const float py = projectile.prev_y + (projectile.y - projectile.prev_y) * alpha +
                         sprite.draw_offset_y;

        SDL_Texture* texture =
            sprite.texture_id != nullptr ? textures_->get(sprite.texture_id) : nullptr;

        if (texture != nullptr) {
            int tex_w = 0;
            int tex_h = 0;
            SDL_QueryTexture(texture, nullptr, nullptr, &tex_w, &tex_h);
            if (tex_w > 0 && tex_h > 0) {
                const int frame_w = sprite.frame_width > 0 ? sprite.frame_width : tex_w;
                const int frame_h = sprite.frame_height > 0 ? sprite.frame_height : tex_h;
                const int frames_in_texture = frame_w > 0 ? std::max(1, tex_w / frame_w) : 1;
                const int frame_count = std::min(sprite.frame_count, frames_in_texture);

                int frame_index = 0;
                if (frame_count > 1 && sprite.anim_period > 0) {
                    frame_index = (projectile.anim_tick / sprite.anim_period) % frame_count;
                }

                int dst_x = 0;
                int dst_y = 0;
                int dst_w = 0;
                int dst_h = 0;
                camera_.world_rect_to_screen(px, py, static_cast<float>(frame_w),
                                             static_cast<float>(frame_h), viewport_w, viewport_h,
                                             dst_x, dst_y, dst_w, dst_h);

                if (dst_w > 0 && dst_h > 0) {
                    SDL_SetTextureAlphaMod(texture, 255);
                    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
                    const SDL_Rect src{frame_index * frame_w, 0, frame_w, frame_h};
                    const SDL_Rect dst{dst_x, dst_y, dst_w, dst_h};

                    if (sprite.rotate_to_velocity &&
                        (projectile.vel_x != 0 || projectile.vel_y != 0)) {
                        const double angle =
                            -std::atan2(static_cast<double>(projectile.vel_y),
                                        static_cast<double>(projectile.vel_x)) *
                            180.0 / M_PI;
                        const SDL_Point center{dst_w / 2, dst_h / 2};
                        SDL_RenderCopyEx(renderer_, texture, &src, &dst, angle, &center,
                                         SDL_FLIP_NONE);
                    } else {
                        SDL_RenderCopy(renderer_, texture, &src, &dst);
                    }
                    continue;
                }
            }
        }

        int dst_x = 0;
        int dst_y = 0;
        int dst_w = 0;
        int dst_h = 0;
        camera_.world_rect_to_screen(px, py, projectile.width, projectile.height, viewport_w,
                                     viewport_h, dst_x, dst_y, dst_w, dst_h);

        if (projectile.kind == sim::ProjectileKind::Plasma) {
            SDL_SetRenderDrawColor(renderer_, 80, 200, 255, 240);
        } else if (projectile.kind == sim::ProjectileKind::Rocket ||
                   projectile.kind == sim::ProjectileKind::SkelFire) {
            SDL_SetRenderDrawColor(renderer_, 255, 140, 40, 240);
        } else if (projectile.kind == sim::ProjectileKind::Bfg) {
            SDL_SetRenderDrawColor(renderer_, 80, 255, 80, 240);
        } else if (projectile.kind == sim::ProjectileKind::ImpFire) {
            SDL_SetRenderDrawColor(renderer_, 255, 200, 80, 240);
        } else if (projectile.kind == sim::ProjectileKind::CacoFire) {
            SDL_SetRenderDrawColor(renderer_, 255, 80, 160, 240);
        } else if (projectile.kind == sim::ProjectileKind::BaronFire ||
                   projectile.kind == sim::ProjectileKind::MancubFire) {
            SDL_SetRenderDrawColor(renderer_, 180, 80, 255, 240);
        } else if (projectile.kind == sim::ProjectileKind::BspFire) {
            SDL_SetRenderDrawColor(renderer_, 120, 220, 255, 240);
        } else if (projectile.kind == sim::ProjectileKind::Flame) {
            SDL_SetRenderDrawColor(renderer_, 255, 120, 40, 220);
        } else if (projectile.kind == sim::ProjectileKind::ShotgunPellet) {
            SDL_SetRenderDrawColor(renderer_, 255, 220, 120, 240);
        } else {
            SDL_SetRenderDrawColor(renderer_, 255, 255, 200, 240);
        }
        const SDL_Rect rect{dst_x, dst_y, dst_w, dst_h};
        SDL_RenderFillRect(renderer_, &rect);
    }
}

void MapViewer::draw_items(int viewport_w, int viewport_h, bool monster_drops_only) {
    if (free_camera_) {
        return;
    }

    SDL_Texture* respawn_texture = textures_->get("tex.ui.itemrespawn");

    for (const auto& item : world_.items().items()) {
        if (item.dropped != monster_drops_only) {
            continue;
        }
        if (!item.active && item.respawn_anim_tick <= 0) {
            continue;
        }

        int dst_x = 0;
        int dst_y = 0;
        int dst_w = 0;
        int dst_h = 0;
        camera_.world_rect_to_screen(item.x, item.y, item.width, item.height, viewport_w,
                                     viewport_h, dst_x, dst_y, dst_w, dst_h);

        if (item.respawn_anim_tick > 0 && respawn_texture != nullptr) {
            const int elapsed = sim::kItemRespawnAnimTotalTicks - item.respawn_anim_tick;
            const int frame = std::min(
                elapsed / sim::kItemRespawnAnimFramePeriod, sim::kItemRespawnAnimFrames - 1);
            const int frame_size = 32;

            int anim_dst_x = 0;
            int anim_dst_y = 0;
            int anim_dst_w = 0;
            int anim_dst_h = 0;
            const float center_x = item.x + item.width * 0.5f - static_cast<float>(frame_size) * 0.5f;
            const float center_y =
                item.y + item.height * 0.5f - static_cast<float>(frame_size) * 0.5f;
            camera_.world_rect_to_screen(center_x, center_y, static_cast<float>(frame_size),
                                         static_cast<float>(frame_size), viewport_w, viewport_h,
                                         anim_dst_x, anim_dst_y, anim_dst_w, anim_dst_h);
            if (anim_dst_w > 0 && anim_dst_h > 0) {
                SDL_SetTextureAlphaMod(respawn_texture, 255);
                SDL_SetTextureBlendMode(respawn_texture, SDL_BLENDMODE_BLEND);
                const SDL_Rect src{frame * frame_size, 0, frame_size, frame_size};
                const SDL_Rect anim_dst{anim_dst_x, anim_dst_y, anim_dst_w, anim_dst_h};
                SDL_RenderCopy(renderer_, respawn_texture, &src, &anim_dst);
            }
        }

        if (!item.active) {
            continue;
        }

        if (dst_w <= 0 || dst_h <= 0) {
            continue;
        }

        const SDL_Rect dst{dst_x, dst_y, dst_w, dst_h};
        const auto sprite = map::item_sprite(item.type);
        SDL_Texture* texture =
            sprite.texture_id != nullptr ? textures_->get(sprite.texture_id) : nullptr;

        if (texture != nullptr) {
            int tex_w = 0;
            int tex_h = 0;
            SDL_QueryTexture(texture, nullptr, nullptr, &tex_w, &tex_h);
            if (tex_w > 0 && tex_h > 0) {
                const int frame_w = sprite.frame_width > 0 ? sprite.frame_width : tex_w;
                const int frame_h = sprite.frame_height > 0 ? sprite.frame_height : tex_h;
                const int frame_index =
                    sprite.frame_count > 1 && sprite.anim_period > 0
                        ? (item.anim_tick / sprite.anim_period) % sprite.frame_count
                        : 0;

                int sprite_dst_x = 0;
                int sprite_dst_y = 0;
                int sprite_dst_w = 0;
                int sprite_dst_h = 0;
                camera_.world_rect_to_screen(item.x, item.y, static_cast<float>(frame_w),
                                             static_cast<float>(frame_h), viewport_w, viewport_h,
                                             sprite_dst_x, sprite_dst_y, sprite_dst_w,
                                             sprite_dst_h);

                if (sprite_dst_w > 0 && sprite_dst_h > 0) {
                    SDL_SetTextureAlphaMod(texture, 255);
                    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
                    const SDL_Rect src{frame_index * frame_w, 0, frame_w, frame_h};
                    const SDL_Rect sprite_dst{sprite_dst_x, sprite_dst_y, sprite_dst_w,
                                              sprite_dst_h};
                    SDL_RenderCopy(renderer_, texture, &src, &sprite_dst);
                    continue;
                }
            }
        }

        const auto type = item.type;
        if (type == map::ItemType::MedkitSmall || type == map::ItemType::MedkitLarge ||
            type == map::ItemType::MedkitBlack) {
            SDL_SetRenderDrawColor(renderer_, 80, 220, 120, 220);
        } else if (type == map::ItemType::ArmorGreen) {
            SDL_SetRenderDrawColor(renderer_, 80, 200, 80, 220);
        } else if (type == map::ItemType::ArmorBlue) {
            SDL_SetRenderDrawColor(renderer_, 80, 140, 255, 220);
        } else if (type == map::ItemType::KeyRed) {
            SDL_SetRenderDrawColor(renderer_, 220, 60, 60, 220);
        } else if (type == map::ItemType::KeyGreen) {
            SDL_SetRenderDrawColor(renderer_, 60, 220, 60, 220);
        } else if (type == map::ItemType::KeyBlue) {
            SDL_SetRenderDrawColor(renderer_, 60, 120, 255, 220);
        } else if (type == map::ItemType::Invul) {
            SDL_SetRenderDrawColor(renderer_, 255, 80, 80, 220);
        } else if (type == map::ItemType::Invis) {
            SDL_SetRenderDrawColor(renderer_, 180, 180, 255, 180);
        } else if (type == map::ItemType::Suit) {
            SDL_SetRenderDrawColor(renderer_, 80, 220, 180, 220);
        } else if (type >= map::ItemType::AmmoBullets && type <= map::ItemType::AmmoBackpack) {
            SDL_SetRenderDrawColor(renderer_, 240, 200, 60, 220);
        } else if (type == map::ItemType::SphereBlue || type == map::ItemType::SphereWhite) {
            SDL_SetRenderDrawColor(renderer_, 220, 80, 220, 220);
        } else if (type >= map::ItemType::WeaponSaw && type <= map::ItemType::WeaponFlamethrower) {
            SDL_SetRenderDrawColor(renderer_, 80, 200, 255, 220);
        } else {
            SDL_SetRenderDrawColor(renderer_, 180, 180, 180, 200);
        }

        SDL_RenderFillRect(renderer_, &dst);
        SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 180);
        SDL_RenderDrawRect(renderer_, &dst);
    }
}

void MapViewer::draw_targets(int viewport_w, int viewport_h) {
    if (free_camera_) {
        return;
    }

    for (const auto& target : world_.targets()) {
        const bool draw_corpse = target.is_monster() &&
                                 (target.is_corpse() || target.is_reviving());
        if (!target.alive() && !draw_corpse) {
            continue;
        }

        const float alpha = fixed_timestep_.render_alpha();
        const float draw_hitbox_x = draw_corpse ? target.x : target.render_x(alpha);
        const float draw_hitbox_y = draw_corpse ? target.y : target.render_y(alpha);

        map::MonsterSprite sprite{};
        if (target.is_monster()) {
            if (target.is_corpse() || target.is_reviving()) {
                sprite = map::monster_corpse_sprite(target.monster_type, target.facing_left);
            } else if (!target.is_awake) {
                sprite = map::monster_sleep_sprite(target.monster_type, target.facing_left);
            } else {
                sprite = map::monster_sprite(target.monster_type, target.facing_left);
            }
        }

        const char* texture_key = sprite.texture_id;
        if (target.facing_left && sprite.texture_id_left != nullptr) {
            texture_key = sprite.texture_id_left;
        }
        SDL_Texture* texture =
            texture_key != nullptr ? textures_->get(texture_key) : nullptr;

        if (texture != nullptr) {
            int tex_w = 0;
            int tex_h = 0;
            SDL_QueryTexture(texture, nullptr, nullptr, &tex_w, &tex_h);
            if (tex_w > 0 && tex_h > 0) {
                const int frame_w = sprite.frame_width > 0 ? sprite.frame_width : tex_w;
                const int frame_h = sprite.frame_height > 0 ? sprite.frame_height : tex_h;
                const int frames_in_texture =
                    frame_w > 0 ? std::max(1, tex_w / frame_w) : 1;
                const int frame_count = std::min(sprite.frame_count, frames_in_texture);

                int frame_index = 0;
                if (frame_count > 1 && sprite.anim_period > 0) {
                    frame_index = (target.anim_tick / sprite.anim_period) % frame_count;
                } else if (draw_corpse && frame_count > 1) {
                    frame_index = frame_count - 1;
                }

                const auto placement = map::monster_sprite_placement(
                    target.monster_type, draw_hitbox_x, draw_hitbox_y, target.width, sprite,
                    target.facing_left);

                int sprite_dst_x = 0;
                int sprite_dst_y = 0;
                int sprite_dst_w = 0;
                int sprite_dst_h = 0;
                camera_.world_rect_to_screen(placement.x, placement.y,
                                             static_cast<float>(frame_w),
                                             static_cast<float>(frame_h), viewport_w, viewport_h,
                                             sprite_dst_x, sprite_dst_y, sprite_dst_w, sprite_dst_h);

                if (sprite_dst_w > 0 && sprite_dst_h > 0) {
                    SDL_SetTextureAlphaMod(texture, 255);
                    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
                    const SDL_Rect src{frame_index * frame_w, 0, frame_w, frame_h};
                    const SDL_Rect sprite_dst{sprite_dst_x, sprite_dst_y, sprite_dst_w,
                                              sprite_dst_h};
                    if (placement.flip_horizontal) {
                        SDL_RenderCopyEx(renderer_, texture, &src, &sprite_dst, 0.0, nullptr,
                                         SDL_FLIP_HORIZONTAL);
                    } else {
                        SDL_RenderCopy(renderer_, texture, &src, &sprite_dst);
                    }
                    continue;
                }
            }
        }

        int dst_x = 0;
        int dst_y = 0;
        int dst_w = 0;
        int dst_h = 0;
        camera_.world_rect_to_screen(draw_hitbox_x, draw_hitbox_y, target.width, target.height,
                                     viewport_w, viewport_h, dst_x, dst_y, dst_w, dst_h);

        if (target.is_monster()) {
            set_monster_draw_color(target.monster_type, renderer_);
        } else {
            SDL_SetRenderDrawColor(renderer_, 200, 48, 48, 220);
        }
        const SDL_Rect rect{dst_x, dst_y, dst_w, dst_h};
        SDL_RenderFillRect(renderer_, &rect);
        SDL_SetRenderDrawColor(renderer_, 255, 200, 200, 255);
        SDL_RenderDrawRect(renderer_, &rect);
    }
}

void MapViewer::spawn_explosion_effect(const events::WorldExplosion& event) {
    const auto sprite = sim::explosion_sprite(event.kind);
    if (sprite.texture_id == nullptr || sprite.frame_count <= 0 || sprite.anim_period <= 0) {
        return;
    }

    WorldEffect effect;
    effect.sprite = sprite;
    effect.x = event.x + sprite.draw_offset_x;
    effect.y = event.y + sprite.draw_offset_y;
    effect.duration_ticks = sprite.frame_count * sprite.anim_period;
    effects_.push_back(effect);
}

void MapViewer::spawn_smoke_effect(const events::WorldSmokePuff& event) {
    const auto sprite = sim::rocket_smoke_sprite();
    if (sprite.texture_id == nullptr || sprite.frame_count <= 0 || sprite.anim_period <= 0) {
        return;
    }

    WorldEffect effect;
    effect.sprite = sprite;
    effect.x = event.x + sprite.draw_offset_x;
    effect.y = event.y + sprite.draw_offset_y;
    effect.duration_ticks = sprite.frame_count * sprite.anim_period;
    effect.alpha = 150;
    effects_.push_back(effect);
}

void MapViewer::spawn_bubble_effect(const events::WorldBubblePuff& event) {
    WorldEffect effect;
    effect.x = event.x;
    effect.y = event.y;
    effect.duration_ticks = 18;
    effect.alpha = 180;
    effect.bubble = true;
    effects_.push_back(effect);
}

void MapViewer::tick_effects() {
    for (auto& effect : effects_) {
        ++effect.anim_tick;
    }

    effects_.erase(std::remove_if(effects_.begin(), effects_.end(),
                                  [](const WorldEffect& effect) {
                                      return effect.anim_tick >= effect.duration_ticks;
                                  }),
                    effects_.end());
}

void MapViewer::draw_effects(int viewport_w, int viewport_h) {
    if (free_camera_) {
        return;
    }

    for (const auto& effect : effects_) {
        if (effect.bubble) {
            int dst_x = 0;
            int dst_y = 0;
            int dst_w = 0;
            int dst_h = 0;
            camera_.world_rect_to_screen(effect.x, effect.y, 8.0f, 8.0f, viewport_w, viewport_h,
                                         dst_x, dst_y, dst_w, dst_h);
            if (dst_w <= 0 || dst_h <= 0) {
                continue;
            }

            SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer_, 220, 235, 255, effect.alpha);
            const SDL_Rect bubble{dst_x, dst_y, dst_w, dst_h};
            SDL_RenderFillRect(renderer_, &bubble);
            continue;
        }

        if (effect.sprite.texture_id == nullptr || textures_ == nullptr) {
            continue;
        }

        SDL_Texture* texture = textures_->get(effect.sprite.texture_id);
        if (texture == nullptr) {
            continue;
        }

        int tex_w = 0;
        int tex_h = 0;
        SDL_QueryTexture(texture, nullptr, nullptr, &tex_w, &tex_h);
        if (tex_w <= 0 || tex_h <= 0) {
            continue;
        }

        const int frame_w = effect.sprite.frame_width > 0 ? effect.sprite.frame_width : tex_w;
        const int frame_h = effect.sprite.frame_height > 0 ? effect.sprite.frame_height : tex_h;
        const int frames_in_texture = frame_w > 0 ? std::max(1, tex_w / frame_w) : 1;
        const int frame_count = std::min(effect.sprite.frame_count, frames_in_texture);
        const int frame_index =
            frame_count > 1 && effect.sprite.anim_period > 0
                ? (effect.anim_tick / effect.sprite.anim_period) % frame_count
                : 0;

        int dst_x = 0;
        int dst_y = 0;
        int dst_w = 0;
        int dst_h = 0;
        camera_.world_rect_to_screen(effect.x, effect.y, static_cast<float>(frame_w),
                                     static_cast<float>(frame_h), viewport_w, viewport_h, dst_x,
                                     dst_y, dst_w, dst_h);
        if (dst_w <= 0 || dst_h <= 0) {
            continue;
        }

        SDL_SetTextureAlphaMod(texture, effect.alpha);
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        const SDL_Rect src{frame_index * frame_w, 0, frame_w, frame_h};
        const SDL_Rect dst{dst_x, dst_y, dst_w, dst_h};
        SDL_RenderCopy(renderer_, texture, &src, &dst);
    }
}

void MapViewer::draw_hud(int viewport_w, int viewport_h) {
    (void)viewport_w;
    const auto& player = world_.player();
    const auto& combat = player.combat();
    const std::string state = player.on_lift()    ? "lift"
                            : player.on_ground() ? "ground"
                            : player.in_water()  ? "water"
                            : player.in_acid()   ? "acid"
                                                 : "air";
    const std::string weapon = sim::weapon_display_name(combat.current_weapon);
    const int ammo = combat.ammo_for_current_weapon();
    std::string charge;
    if (combat.bfg_charge_ticks >= 0) {
        charge = "  [BFG charge " + std::to_string(combat.bfg_charge_ticks) + "]";
    }
    std::string keys;
    if (player.has_key_red()) {
        keys += "R";
    }
    if (player.has_key_green()) {
        keys += "G";
    }
    if (player.has_key_blue()) {
        keys += "B";
    }
  const std::string key_line = keys.empty() ? "" : "  Keys " + keys;
    std::string powerups;
    if (player.has_invul()) {
        powerups += " INV";
    }
    if (player.has_invis()) {
        powerups += " INVIS";
    }
    if (player.has_suit()) {
        powerups += " SUIT";
    }
    if (player.jetpack_active() || player.jet_fuel() > 0) {
        powerups += " JET " + std::to_string(player.jet_fuel());
    }
    const std::string hud = map_.name + "  (" + std::to_string(map_index_ + 1) + "/" +
                            std::to_string(map_list_.size()) + ")  HP " +
                            std::to_string(player.health()) + "/" +
                            std::to_string(sim::PlayerState::kMaxHealth) + "  AP " +
                            std::to_string(player.armor()) + "/" +
                            std::to_string(sim::PlayerState::kArmorLimit) + key_line + powerups +
                            "  |  " +
                            weapon + " " + std::to_string(ammo) + charge +
                            "  |  1-9 weapons  0/B=BFG  -/G=SuperCG  Q prev  wheel zoom  " +
                            state;
    text_.draw(renderer_, hud, 8, viewport_h - 24);
}

void MapViewer::render(int viewport_w, int viewport_h) {
    update_camera_follow(fixed_timestep_.render_alpha());

    SDL_SetRenderDrawColor(renderer_, 20, 15, 26, 255);
    SDL_RenderClear(renderer_);

    draw_sky(viewport_w, viewport_h);
    map_renderer_.draw(renderer_, triggers_.map_view(map_), *textures_, camera_, viewport_w, viewport_h);
    draw_items(viewport_w, viewport_h, false);
    draw_player(viewport_w, viewport_h);
    draw_targets(viewport_w, viewport_h);
    draw_items(viewport_w, viewport_h, true);
    draw_projectiles(viewport_w, viewport_h);
    draw_effects(viewport_w, viewport_h);
#if D2DF_DEBUG_UI
    draw_debug_overlays(viewport_w, viewport_h);
#endif
    draw_hud(viewport_w, viewport_h);
}

#if D2DF_DEBUG_UI
void MapViewer::draw_debug_overlays(int viewport_w, int viewport_h) {
    if (debug_ui_ == nullptr) {
        return;
    }

    const debug::DebugWorldView world{
        world_.player(),
        world_.targets(),
        world_.projectiles(),
        collision_,
        triggers_.map_view(map_),
        triggers_,
        fixed_timestep_.render_alpha(),
    };
    debug_ui_->draw_world(renderer_, camera_, viewport_w, viewport_h, world);
}
#endif

} // namespace d2df
