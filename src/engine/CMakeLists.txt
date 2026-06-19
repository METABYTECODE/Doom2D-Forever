add_library(rivet_engine STATIC
    src/audio/audio_system.cpp
    src/core/application.cpp
    src/core/clock.cpp
    src/core/event_bus.cpp
    src/core/fixed_timestep.cpp
    src/core/game_context.cpp
    src/ecs/world.cpp
    src/input/input_system.cpp
    src/physics/aabb.cpp
    src/physics/fluid_grid.cpp
    src/physics/physics_world.cpp
    src/physics/spatial_grid.cpp
    src/physics/tile_collider.cpp
    src/resources/png_writer.cpp
    src/resources/resource_manager.cpp
    src/scene/scene.cpp
    src/scene/scene_manager.cpp
    src/mod/api.cpp
    src/mod/command_router.cpp
    src/mod/data_query.cpp
    src/mod/event_hub.cpp
    src/platform/bootstrap.cpp
    src/platform/sdl_platform.cpp
    src/render/camera2d.cpp
    src/render/renderer_backend.cpp
    src/render/renderer_factory.cpp
    src/render/sdl_renderer.cpp
)

add_library(rivet::engine ALIAS rivet_engine)

target_include_directories(rivet_engine
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

find_package(Stb REQUIRED)

target_link_libraries(rivet_engine
    PUBLIC
        EnTT::EnTT
        spdlog::spdlog
        fmt::fmt
        SDL2::SDL2
        SDL2_mixer::SDL2_mixer
)

target_include_directories(rivet_engine PRIVATE ${Stb_INCLUDE_DIR})

d2df_set_warnings(rivet_engine)
