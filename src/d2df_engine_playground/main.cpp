#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>

#include <SDL.h>

#include <d2df/core/fixed_timestep.hpp>
#include <d2df/core/log.hpp>
#include <d2df/engine/map/map_loader.hpp>
#include <d2df/engine/render/tile_map_renderer.hpp>
#include <d2df/engine/render/tile_map_render_index.hpp>
#include <d2df/engine/runtime/map_session.hpp>
#include <d2df/map/map_catalog.hpp>
#include <d2df/render/camera2d.hpp>
#include <d2df/render/texture_cache.hpp>
#include <d2df/resources/asset_database.hpp>

namespace {

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;

void print_usage() {
    std::cerr << "Usage: d2df_engine [--content PATH] [--map PATH]\n"
              << "  --content  Content root (default: assets/content)\n"
              << "  --map      Map JSON (default: maps/doom2d/map01.json)\n"
              << "Controls: A/D move, Space jump, C camera, PgUp/PgDn maps, ESC quit\n"
              << "Tip: use run_engine.bat from the repo root (sets --content automatically).\n";
}

bool content_root_ready(const std::filesystem::path& content_root) {
    std::error_code ec;
    return std::filesystem::exists(content_root / "manifest.json", ec);
}

std::filesystem::path resolve_content_root(const std::filesystem::path& requested) {
    if (!requested.empty()) {
        return requested;
    }

    const std::filesystem::path candidates[] = {
        "assets/content",
        "../assets/content",
        "../../assets/content",
    };

    for (const auto& candidate : candidates) {
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec)) {
            return std::filesystem::weakly_canonical(candidate, ec);
        }
    }

    return "assets/content";
}

d2df::engine::MovementInput read_movement_input() {
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    d2df::engine::MovementInput input;
    input.left = keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT];
    input.right = keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT];
    input.jump = keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP];
    return input;
}

void draw_entity_box(SDL_Renderer* renderer, const d2df::render::Camera2D& camera, int viewport_w,
                     int viewport_h, float x, float y, float width, float height, Uint8 r, Uint8 g,
                     Uint8 b) {
    int rect_x = 0;
    int rect_y = 0;
    int rect_w = 0;
    int rect_h = 0;
    camera.world_rect_to_screen(x, y, width, height, viewport_w, viewport_h, rect_x, rect_y, rect_w,
                                rect_h);

    SDL_Rect rect{rect_x, rect_y, rect_w, rect_h};
    SDL_SetRenderDrawColor(renderer, r, g, b, 220);
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &rect);
}

} // namespace

extern "C" int SDL_main(int argc, char* argv[]) {
    std::filesystem::path content_root;
    std::filesystem::path map_rel = "maps/doom2d/map01.json";

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_usage();
            return 0;
        }
        if (arg == "--content" && i + 1 < argc) {
            content_root = argv[++i];
        } else if (arg == "--map" && i + 1 < argc) {
            map_rel = argv[++i];
        } else {
            std::cerr << "Unknown argument: " << arg << '\n';
            print_usage();
            return 1;
        }
    }

    content_root = resolve_content_root(content_root);
    if (!content_root_ready(content_root)) {
        std::cerr << "Content not found at: " << content_root.string() << '\n'
                  << "Run organize_assets.bat or pass --content assets\\content\n";
        return 1;
    }

    const auto map_path = content_root / map_rel;
    if (!std::filesystem::exists(map_path)) {
        std::cerr << "Map not found: " << map_path.string() << '\n';
        return 1;
    }

    auto map_list = d2df::map::list_map_json_files(content_root);
    std::size_t map_index = 0;
    const auto map_path_norm = std::filesystem::weakly_canonical(map_path);
    for (std::size_t i = 0; i < map_list.size(); ++i) {
        std::error_code ec;
        const auto candidate = std::filesystem::weakly_canonical(map_list[i], ec);
        if (!ec && candidate == map_path_norm) {
            map_index = i;
            break;
        }
    }

    d2df::init_logging();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
        return 1;
    }

    SDL_Window* window =
        SDL_CreateWindow("Doom2D Engine Playground", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         kWindowWidth, kWindowHeight, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    d2df::engine::MapSession session;
    d2df::engine::render::TileMapRenderIndex tile_index;
    d2df::engine::map::TileMap tile_map;

    auto reload_map = [&](const std::filesystem::path& path) -> bool {
        try {
            tile_map = d2df::engine::map::load_map_file(path);
        } catch (const std::exception& ex) {
            std::cerr << "Failed to load map: " << ex.what() << '\n';
            return false;
        }
        session.load(std::move(tile_map));
        tile_index.build(session.tile_map());
        const std::string title = "Doom2D Engine — " + session.tile_map().name;
        SDL_SetWindowTitle(window, title.c_str());
        return true;
    };

    if (!reload_map(map_path)) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    d2df::resources::AssetDatabase assets;
    assets.load(content_root);
    d2df::render::TextureCache textures(renderer, assets);

    d2df::engine::render::TileMapRenderer map_renderer;

    d2df::render::Camera2D camera;
    camera.scale = 2;
    bool camera_follow = true;

    d2df::FixedTimestep timestep;
    Uint32 last_ticks = SDL_GetTicks();
    bool running = true;

    while (running) {
        const Uint32 now_ticks = SDL_GetTicks();
        const float frame_dt = static_cast<float>(now_ticks - last_ticks) / 1000.0f;
        last_ticks = now_ticks;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                } else if (event.key.keysym.sym == SDLK_c) {
                    camera_follow = !camera_follow;
                } else if (event.key.keysym.sym == SDLK_PAGEUP && map_list.size() > 1) {
                    map_index = (map_index + map_list.size() - 1) % map_list.size();
                    reload_map(map_list[map_index]);
                } else if (event.key.keysym.sym == SDLK_PAGEDOWN && map_list.size() > 1) {
                    map_index = (map_index + 1) % map_list.size();
                    reload_map(map_list[map_index]);
                }
            }
        }

        const auto movement = read_movement_input();
        timestep.advance(frame_dt);
        while (timestep.consume_fixed_step()) {
            session.fixed_tick(movement);
        }

        int viewport_w = 0;
        int viewport_h = 0;
        SDL_GetRendererOutputSize(renderer, &viewport_w, &viewport_h);

        const auto& player = session.world().player();
        const float render_alpha = timestep.render_alpha();
        if (camera_follow) {
            camera.center_x = player.render_x(render_alpha) + player.width * 0.5f;
            camera.center_y = player.render_y(render_alpha) + player.height * 0.5f;
        }

        SDL_SetRenderDrawColor(renderer, 16, 16, 24, 255);
        SDL_RenderClear(renderer);

        map_renderer.draw(renderer, session.tile_map(), textures, camera, viewport_w, viewport_h,
                          &tile_index);

        const float alpha = render_alpha;
        draw_entity_box(renderer, camera, viewport_w, viewport_h, player.render_x(alpha),
                        player.render_y(alpha), player.width, player.height, 80, 200, 255);

        const auto& registry = session.world().registry();
        for (const auto entity : session.world().entity_entities()) {
            const auto& transform = registry.get<d2df::ecs::Transform>(entity);
            const auto& collider = registry.get<d2df::engine::ecs::Collider>(entity);
            draw_entity_box(renderer, camera, viewport_w, viewport_h, transform.x, transform.y,
                            collider.width, collider.height, 200, 80, 80);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
