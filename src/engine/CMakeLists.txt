add_library(d2df_engine STATIC
    src/ecs/engine_world.cpp
    src/map/entity_catalog.cpp
    src/map/legacy_map_adapter.cpp
    src/map/map_loader.cpp
    src/map/tile_map_json.cpp
    src/map/tile_map_sync.cpp
    src/physics/collision_world.cpp
    src/physics/entity_collision.cpp
    src/render/tile_map_renderer.cpp
    src/render/tile_map_render_index.cpp
    src/runtime/map_session.cpp
)

add_library(d2df::engine ALIAS d2df_engine)

target_include_directories(d2df_engine
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/src/d2df/include"
)

target_link_libraries(d2df_engine
    PUBLIC
        d2df::core
        d2df::map
        d2df::sim
        d2df::render
        EnTT::EnTT
)

d2df_set_warnings(d2df_engine)
