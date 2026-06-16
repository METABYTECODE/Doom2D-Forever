add_executable(d2df-wad-ls main.cpp)
target_link_libraries(d2df-wad-ls PRIVATE d2df::archive)
d2df_set_warnings(d2df-wad-ls)
set_target_properties(d2df-wad-ls PROPERTIES OUTPUT_NAME "d2df-wad-ls")
