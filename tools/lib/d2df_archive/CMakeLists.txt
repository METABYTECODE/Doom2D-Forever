add_library(d2df_archive STATIC
    src/archive.cpp
    src/binary_reader.cpp
    src/content_validate.cpp
    src/extract_pipeline.cpp
    src/ffmpeg_util.cpp
    src/format_sniff.cpp
    src/json_util.cpp
    src/legacy_map.cpp
    src/map_enum_names.cpp
    src/map_import_pipeline.cpp
    src/map_json.cpp
    src/nested_resource.cpp
    src/organize_pipeline.cpp
    src/slug.cpp
    src/staging_manifest.cpp
    src/text_encoding.cpp
    src/zlib_util.cpp
)

add_library(d2df::archive ALIAS d2df_archive)

target_include_directories(d2df_archive
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(d2df_archive
    PUBLIC
        ZLIB::ZLIB
        spdlog::spdlog
        fmt::fmt
        d2df::resources
)

d2df_set_warnings(d2df_archive)
