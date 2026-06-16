add_executable(d2df_server main.cpp)

target_link_libraries(d2df_server PRIVATE d2df::core)

d2df_set_warnings(d2df_server)

set_target_properties(d2df_server PROPERTIES
    OUTPUT_NAME "d2df_server"
)
