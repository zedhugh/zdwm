cmake_minimum_required(VERSION 3.24)

# use GNU23
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(APP_NAME "zdwm")
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}")

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

add_executable(${APP_NAME}
    ${SOURCE_DIR}/backend/output_utils.c
    ${SOURCE_DIR}/backend/x11/backend.c
    ${SOURCE_DIR}/backend/x11/event.c
    ${SOURCE_DIR}/backend/x11/window.c

    ${SOURCE_DIR}/bar/cell.c
    ${SOURCE_DIR}/bar/workspaces.c

    ${SOURCE_DIR}/base/color.c
    ${SOURCE_DIR}/base/log.c
    ${SOURCE_DIR}/base/process.c
    ${SOURCE_DIR}/base/window_list.c

    ${SOURCE_DIR}/config/defaults.c
    ${SOURCE_DIR}/config/loader.c
    ${SOURCE_DIR}/config/runtime_config.c

    ${SOURCE_DIR}/core/binding.c
    ${SOURCE_DIR}/core/command_buffer.c
    ${SOURCE_DIR}/core/event.c
    ${SOURCE_DIR}/core/layer.c
    ${SOURCE_DIR}/core/layout.c
    ${SOURCE_DIR}/core/listeners.c
    ${SOURCE_DIR}/core/plan.c
    ${SOURCE_DIR}/core/policy.c
    ${SOURCE_DIR}/core/rules.c
    ${SOURCE_DIR}/core/runtime.c
    ${SOURCE_DIR}/core/state.c
    ${SOURCE_DIR}/core/window.c

    ${SOURCE_DIR}/layouts/fair.c
    ${SOURCE_DIR}/layouts/fullscreen.c
    ${SOURCE_DIR}/layouts/maximize.c

    ${SOURCE_DIR}/main.c
)

# target_compile_definitions(${APP_NAME}
#     PRIVATE LOG_FILE="$ENV{HOME}/${APP_NAME}-log.txt"
# )

find_package(PkgConfig REQUIRED)

pkg_check_modules(deps REQUIRED
    xcb
    xcb-aux
    xcb-icccm
    xcb-keysyms
    xcb-randr
    xcb-xfixes
    xcb-xinerama
    xkbcommon

    cairo
    cairo-xcb
)

target_include_directories(${APP_NAME} SYSTEM
    PRIVATE ${deps_INCLUDE_DIRS}
    PRIVATE ${INCLUDE_DIR}
)
target_include_directories(${APP_NAME}
    PRIVATE ${SOURCE_DIR}
    PRIVATE ${BUILD_DIR}
)

target_link_libraries(${APP_NAME}
    PRIVATE m
    PRIVATE ${deps_LIBRARIES}
)
