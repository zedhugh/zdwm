cmake_minimum_required(VERSION 3.24)

set(APP_NAME "zdwm")
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

add_compile_definitions(LOG_FILE="$ENV{HOME}/zdwm-log.txt")

# generate compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# compile flags
add_compile_options(-g)
add_compile_options(-Wall)
# add_link_options(-fsanitize=address)

project(${APP_NAME}
    VERSION 0.0.1
    LANGUAGES C
)

add_executable(${APP_NAME} main.c)

# use C23
set_property(TARGET ${APP_NAME} PROPERTY C_STANDARD 23)
