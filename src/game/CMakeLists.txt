add_library(rivet_game STATIC
    src/bounce_game.cpp
    src/bounce_scene.cpp
    src/content/content_paths.cpp
    src/game_catalog.cpp
    src/level/level_loader.cpp
    src/level/level_saver.cpp
    src/level/level_spawner.cpp
    src/level_game.cpp
    src/level_scene.cpp
    src/sandbox_game.cpp
    src/sandbox_scene.cpp
    src/tileset/tileset_catalog.cpp
)

add_library(rivet::game ALIAS rivet_game)

target_include_directories(rivet_game
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

find_package(nlohmann_json CONFIG REQUIRED)

target_link_libraries(rivet_game
    PUBLIC
        rivet::engine
        nlohmann_json::nlohmann_json
)

d2df_set_warnings(rivet_game)
