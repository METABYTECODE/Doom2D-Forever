add_executable(d2df-extract main.cpp)
target_link_libraries(d2df-extract PRIVATE d2df::archive d2df::core)
d2df_set_warnings(d2df-extract)
set_target_properties(d2df-extract PROPERTIES OUTPUT_NAME "d2df-extract")
