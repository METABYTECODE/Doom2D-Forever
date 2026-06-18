add_executable(rivet_playground main.cpp)

target_link_libraries(rivet_playground
    PRIVATE
        rivet::game
)

d2df_set_warnings(rivet_playground)

set_target_properties(rivet_playground PROPERTIES
    OUTPUT_NAME "rivet_engine"
)
