cmake_minimum_required(VERSION 3.12.4)
project(dashboard C)

set(CMAKE_C_STANDARD 11)

# lvgl finds lv_conf.h in this dir (created from lv_conf_template.h by the recipe)
add_compile_definitions(LV_CONF_INCLUDE_SIMPLE)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(lvgl)

add_executable(dashboard main.c)
target_link_libraries(dashboard lvgl m pthread)

install(TARGETS dashboard RUNTIME DESTINATION bin)
