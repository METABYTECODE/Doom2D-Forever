add_executable(d2df-organize main.cpp)
target_link_libraries(d2df-organize PRIVATE d2df::archive d2df::core)
d2df_set_warnings(d2df-organize)
set_target_properties(d2df-organize PROPERTIES OUTPUT_NAME "d2df-organize")
