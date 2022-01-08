idf_component_register(SRCS "esp_smart_home.cc"
                    INCLUDE_DIRS ".")

include_directories($CMAKE_CURRENT_SOURCE_DIR)
add_subdirectory(core)
target_link_libraries(${COMPONENT_LIB} PUBLIC core)
