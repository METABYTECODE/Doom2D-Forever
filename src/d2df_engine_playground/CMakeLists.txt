add_executable(d2df_engine_playground main.cpp)

target_link_libraries(d2df_engine_playground
    PRIVATE
        d2df::engine
        d2df::map
        d2df::render
        d2df::resources
        d2df::core_impl
        SDL2::SDL2main
        SDL2::SDL2
)

d2df_set_warnings(d2df_engine_playground)

set_target_properties(d2df_engine_playground PROPERTIES
    OUTPUT_NAME "d2df_engine"
    WIN32_EXECUTABLE TRUE
)
