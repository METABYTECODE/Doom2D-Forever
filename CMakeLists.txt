cmake_minimum_required(VERSION 3.25)

project(Doom2DForever
    VERSION 0.1.0
    DESCRIPTION "Modern C++ rewrite of Doom2D Forever"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(D2DF_BUILD_TESTS "Build unit tests" ON)
option(D2DF_BUILD_TOOLS "Build asset/import tools" ON)
option(D2DF_DEBUG_UI "Build ImGui debug overlay in the client" ON)
option(D2DF_LEAK_CHECK "Enable MSVC CRT leak check on exit (Debug builds)" ON)
option(D2DF_HEADLESS_SERVER "Build server without SDL window" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(cmake/CompilerWarnings.cmake)

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_mixer CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)

if(D2DF_BUILD_TESTS)
    find_package(Catch2 3 CONFIG REQUIRED)
endif()

add_subdirectory(src/d2df)

if(D2DF_BUILD_TOOLS)
    find_package(ZLIB REQUIRED)
    add_subdirectory(tools)
endif()

add_subdirectory(src/d2df_client)

if(NOT D2DF_HEADLESS_SERVER)
    # Server stub links core library only (no SDL window in Phase 0).
endif()
add_subdirectory(src/d2df_server)

if(D2DF_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

message(STATUS "Doom2D Forever ${PROJECT_VERSION} — Phase 3/4 viewer + player prototype")
