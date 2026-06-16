add_executable(d2df-map-import main.cpp)
target_link_libraries(d2df-map-import PRIVATE d2df::archive d2df::core d2df::resources)
d2df_set_warnings(d2df-map-import)
set_target_properties(d2df-map-import PROPERTIES OUTPUT_NAME "d2df-map-import")
