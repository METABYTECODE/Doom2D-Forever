#include <d2df/app/map_viewer.hpp>

#include <SDL.h>
#include <spdlog/spdlog.h>

#include <cmath>

#include <d2df/core/event_bus.hpp>
#include <d2df/core/game_events.hpp>
#include <d2df/map/map_catalog.hpp>
#include <d2df/map/map_json_loader.hpp>

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

MapViewer::MapViewer(SDL_Renderer* renderer, std::filesystem::path content_root,
                     std::filesystem::path map_path, EventBus* events)
    : renderer_(renderer)
    , content_root_(std::move(content_root))
    , events_(events) {
    assets_.load(content_root_);
    text_.init(content_root_);
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

    spdlog::info("Map [{}/{}]: {} ({}x{}, {} panels, {} areas)", map_index_ + 1,
                 map_list_.size(), map_.name, map_.size.width, map_.size.height, map_.panels.size(),
                 map_.areas.size());
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
    default:
        break;
    }

    switch (sym) {
    case SDLK_EQUALS:
    case SDLK_PLUS:
    case SDLK_KP_PLUS:
        camera_.zoom_in();
        break;
    case SDLK_MINUS:
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
        break;
    case SDLK_e:
    case SDLK_f:
        if (!key_use_) {
            key_use_edge_ = true;
        }
        key_use_ = true;
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
        break;
    case SDLK_e:
    case SDLK_f:
        key_use_ = false;
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

    sim::PlayerInput input{key_left_, key_right_, key_jump_};
    auto& player = world_.player();
    collision_.build_from_panels(triggers_.panels());
    world_.fixed_update(collision_, input, events_);
    triggers_.update(player, key_use_edge_, events_);
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

    update_camera_follow();
}

void MapViewer::update_camera_follow() {
    const auto& player = world_.player();
    camera_.center_x = player.x + player.width * 0.5f;
    camera_.center_y = player.y + player.height * 0.5f;
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

void MapViewer::draw_player(int viewport_w, int viewport_h) {
    if (free_camera_) {
        return;
    }

    const auto& player = world_.player();
    const float alpha = fixed_timestep_.render_alpha();
    const float px = player.render_x(alpha);
    const float py = player.render_y(alpha);

    int dst_x = 0;
    int dst_y = 0;
    int dst_w = 0;
    int dst_h = 0;
    camera_.world_rect_to_screen(px, py, player.width, player.height, viewport_w, viewport_h, dst_x,
                                 dst_y, dst_w, dst_h);

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

void MapViewer::draw_hud(int viewport_w, int viewport_h) {
    (void)viewport_w;
    const auto& player = world_.player();
    const std::string state = player.on_lift()    ? "lift"
                            : player.on_ground() ? "ground"
                            : player.in_water()  ? "water"
                            : player.in_acid()   ? "acid"
                                                 : "air";
    const std::string hud = map_.name + "  (" + std::to_string(map_index_ + 1) + "/" +
                            std::to_string(map_list_.size()) + ")  HP " +
                            std::to_string(player.health()) + "/" +
                            std::to_string(sim::PlayerState::kMaxHealth) +
                            "  |  A/D move  Space jump  E use  PgUp/PgDn maps  C camera  " + state;
    text_.draw(renderer_, hud, 8, viewport_h - 24);
}

void MapViewer::render(int viewport_w, int viewport_h) {
    SDL_SetRenderDrawColor(renderer_, 20, 15, 26, 255);
    SDL_RenderClear(renderer_);

    draw_sky(viewport_w, viewport_h);
    map_renderer_.draw(renderer_, triggers_.map_view(map_), *textures_, camera_, viewport_w, viewport_h);
    draw_player(viewport_w, viewport_h);
    draw_hud(viewport_w, viewport_h);
}

} // namespace d2df
