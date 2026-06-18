include(CTest)

add_executable(d2df_tests
    test_event_bus.cpp
    test_services.cpp
    test_archive.cpp
    test_organize.cpp
    test_map_import.cpp
    test_map_viewer.cpp
    test_perf.cpp
    test_sim.cpp
    test_engine.cpp
    test_game.cpp
)

target_link_libraries(d2df_tests
    PRIVATE
        d2df::core
        d2df::core_impl
        d2df::sim
        d2df::render
        d2df::engine
        d2df::game
        Catch2::Catch2WithMain
)

if(TARGET d2df::archive)
    target_link_libraries(d2df_tests PRIVATE d2df::archive d2df::resources d2df::map)
    target_compile_definitions(d2df_tests PRIVATE D2DF_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
endif()

d2df_set_warnings(d2df_tests)

include(Catch)
catch_discover_tests(d2df_tests)
