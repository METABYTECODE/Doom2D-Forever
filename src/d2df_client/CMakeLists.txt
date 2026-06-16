add_executable(d2df_client main.cpp)

target_link_libraries(d2df_client PRIVATE d2df::app SDL2::SDL2main)

d2df_set_warnings(d2df_client)

set_target_properties(d2df_client PROPERTIES
    OUTPUT_NAME "d2df"
    WIN32_EXECUTABLE TRUE
)
