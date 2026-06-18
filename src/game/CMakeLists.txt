add_library(d2df_game STATIC
    src/game_map_bundle.cpp
    src/gameplay_session.cpp
)

add_library(d2df::game ALIAS d2df_game)

target_include_directories(d2df_game
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/src/d2df/include"
        "${CMAKE_SOURCE_DIR}/src/engine/include"
)

target_link_libraries(d2df_game
    PUBLIC
        d2df::engine
        d2df::map
        d2df::sim
)

d2df_set_warnings(d2df_game)
