add_executable(d2df_client main.cpp)

target_link_libraries(d2df_client PRIVATE d2df::app SDL2::SDL2main)

if(MSVC AND D2DF_LEAK_CHECK)
    target_compile_definitions(d2df_client PRIVATE D2DF_LEAK_CHECK=1)
endif()

d2df_set_warnings(d2df_client)

set_target_properties(d2df_client PROPERTIES
    OUTPUT_NAME "d2df"
    WIN32_EXECUTABLE TRUE
)
