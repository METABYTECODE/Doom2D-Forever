add_executable(d2df-content-validate main.cpp)
target_link_libraries(d2df-content-validate PRIVATE d2df::archive d2df::core d2df::resources)
d2df_set_warnings(d2df-content-validate)
set_target_properties(d2df-content-validate PROPERTIES OUTPUT_NAME "d2df-content-validate")
