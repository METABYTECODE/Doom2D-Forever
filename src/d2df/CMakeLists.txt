add_library(d2df_core INTERFACE)

add_library(d2df::core ALIAS d2df_core)

target_include_directories(d2df_core
    INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(d2df_core
    INTERFACE
        spdlog::spdlog
        fmt::fmt
)

add_library(d2df_core_impl STATIC
    src/core/fixed_timestep.cpp
)

add_library(d2df::core_impl ALIAS d2df_core_impl)

target_link_libraries(d2df_core_impl
    PUBLIC
        d2df::core
)

d2df_set_warnings(d2df_core_impl)

add_library(d2df_resources STATIC
    src/resources/asset_database.cpp
    src/resources/content_catalog.cpp
    src/resources/legacy_ref.cpp
    "${CMAKE_SOURCE_DIR}/tools/lib/d2df_archive/src/text_encoding.cpp"
)

add_library(d2df::resources ALIAS d2df_resources)

target_include_directories(d2df_resources
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/tools/lib/d2df_archive/include"
)

target_link_libraries(d2df_resources
    PUBLIC
        d2df::core
)

d2df_set_warnings(d2df_resources)

add_library(d2df_map STATIC
    src/map/area_types.cpp
    src/map/item_types.cpp
    src/map/key_types.cpp
    src/map/map_catalog.cpp
    src/map/map_json_loader.cpp
    src/map/map_spawn.cpp
    src/map/monster_types.cpp
    src/map/panel_types.cpp
    src/map/trigger_types.cpp
)

add_library(d2df::map ALIAS d2df_map)

target_include_directories(d2df_map
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(d2df_map
    PUBLIC
        d2df::core
)

d2df_set_warnings(d2df_map)

add_library(d2df_sim STATIC
    src/ecs/game_world.cpp
    src/sim/combat_common.cpp
    src/sim/item_system.cpp
    src/sim/map_collision.cpp
    src/sim/monster_system.cpp
    src/sim/player_corpse_system.cpp
    src/sim/player_state.cpp
    src/sim/lift_system.cpp
    src/sim/projectile_system.cpp
    src/sim/projectile_types.cpp
    src/sim/player_types.cpp
    src/sim/effect_types.cpp
    src/sim/shootable_target.cpp
    src/sim/trigger_system.cpp
    src/sim/weapon_system.cpp
    src/sim/weapon_types.cpp
)

add_library(d2df::sim ALIAS d2df_sim)

target_include_directories(d2df_sim
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(d2df_sim
    PUBLIC
        d2df::core
        d2df::map
        EnTT::EnTT
)

d2df_set_warnings(d2df_sim)

add_library(d2df_audio STATIC
    src/audio/player_continuous_sfx.cpp
    src/audio/sound_system.cpp
)

add_library(d2df::audio ALIAS d2df_audio)

target_include_directories(d2df_audio
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(d2df_audio
    PUBLIC
        d2df::core
        d2df::resources
        d2df::sim
        SDL2_mixer::SDL2_mixer
)

d2df_set_warnings(d2df_audio)

add_library(d2df_render STATIC
    src/render/camera2d.cpp
    src/render/hud_layout.cpp
    src/render/map_renderer.cpp
    src/render/text_renderer.cpp
    src/render/texture_cache.cpp
)

add_library(d2df::render ALIAS d2df_render)

target_include_directories(d2df_render
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE
        "${CMAKE_SOURCE_DIR}/src/tools/png2map"
)

target_link_libraries(d2df_render
    PUBLIC
        d2df::core
        d2df::map
        d2df::resources
        SDL2::SDL2
        SDL2_ttf::SDL2_ttf
)

d2df_set_warnings(d2df_render)

if(D2DF_DEBUG_UI)
    include(FetchContent)
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.91.9
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(imgui)

    add_library(imgui_sdl2 STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp
    )

    target_include_directories(imgui_sdl2
        PUBLIC
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
    )

    target_link_libraries(imgui_sdl2 PUBLIC SDL2::SDL2)

    add_library(d2df_debug STATIC
        src/debug/debug_draw.cpp
        src/debug/debug_ui.cpp
        src/debug/panels/collision_debug_panel.cpp
        src/debug/panels/tiles_debug_panel.cpp
    )

    add_library(d2df::debug ALIAS d2df_debug)

    target_include_directories(d2df_debug
        PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )

    target_link_libraries(d2df_debug
        PUBLIC
            d2df::core
            d2df::map
            d2df::sim
            d2df::render
            SDL2::SDL2
            imgui_sdl2
    )

    d2df_set_warnings(d2df_debug)
    d2df_set_warnings(imgui_sdl2)
endif()

add_library(d2df_app STATIC
    src/app/content_paths.cpp
    src/app/game_app.cpp
    src/app/map_viewer.cpp
)

add_library(d2df::app ALIAS d2df_app)

target_link_libraries(d2df_app
    PUBLIC
        d2df::core
        d2df::core_impl
        d2df::render
        d2df::map
        d2df::sim
        d2df::audio
        d2df::resources
        SDL2::SDL2
)

if(D2DF_DEBUG_UI)
    target_compile_definitions(d2df_app PUBLIC D2DF_DEBUG_UI=1)
    target_link_libraries(d2df_app PUBLIC d2df::debug)
else()
    target_compile_definitions(d2df_app PUBLIC D2DF_DEBUG_UI=0)
endif()

if(WIN32)
    target_compile_definitions(d2df_app PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

d2df_set_warnings(d2df_app)
