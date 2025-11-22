cmake_minimum_required(VERSION 3.24)

set(APP_NAME "zdwm")
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}")

add_compile_definitions(LOG_FILE="$ENV{HOME}/${APP_NAME}-log.txt")

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
    ${SOURCE_DIR}/wm.c
    ${SOURCE_DIR}/utils.c
    ${SOURCE_DIR}/buffer.c
    ${SOURCE_DIR}/backtrace.c
    ${SOURCE_DIR}/monitor.c
    ${SOURCE_DIR}/client.c
    ${SOURCE_DIR}/text.c
    ${SOURCE_DIR}/color.c
    ${SOURCE_DIR}/xcursor.c
    ${SOURCE_DIR}/xwindow.c
    ${SOURCE_DIR}/event.c
    ${SOURCE_DIR}/action.c
    ${SOURCE_DIR}/xkb.c
    ${SOURCE_DIR}/atoms.c
)

# use C23
set_property(TARGET ${APP_NAME} PROPERTY C_STANDARD 23)

find_package(PkgConfig REQUIRED)

pkg_check_modules(deps REQUIRED
    glib-2.0

    x11
    xcb
    xcb-aux
    xcb-cursor
    xcb-keysyms
    xcb-randr
    xcb-xfixes
    xcb-xinerama
    xcb-xkb
    xkbcommon-x11

    cairo
    cairo-xcb
    pango
    pangocairo
)

target_include_directories(${APP_NAME} SYSTEM
    PRIVATE ${deps_INCLUDE_DIRS}
)
target_include_directories(${APP_NAME}
    PRIVATE ${SOURCE_DIR}
    PRIVATE ${BUILD_DIR}
)

target_link_libraries(${APP_NAME}
    PRIVATE ${deps_LIBRARIES}
)


## 配置项
# Check for backtrace_symbols()
include(CheckSymbolExists)
check_symbol_exists(backtrace_symbols execinfo.h HAS_EXECINFO)
if(NOT HAS_EXECINFO)
    find_library(LIB_EXECINFO execinfo)
    if(LIB_EXECINFO)
        set(HAS_EXECINFO TRUE)
        target_link_libraries(${APP_NAME} ${LIB_EXECINFO})
    endif()
endif()
if(HAS_EXECINFO)
    message(STATUS "checking for execinfo -- found")
else()
    message(STATUS "checking for execinfo -- not found")
endif()

configure_file("${SOURCE_DIR}/app.h.in"
    "${BUILD_DIR}/app.h"
    ESCAPE_QUOTES
    @ONLY
)

set(ATOM_LIST_FILE "${SOURCE_DIR}/atoms-list.txt")
add_custom_command(
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/build-utils/atoms-extern.sh
    ARGS ${ATOM_LIST_FILE} > ${BUILD_DIR}/atoms-extern.h
    OUTPUT ${BUILD_DIR}/atoms-extern.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${ATOM_LIST_FILE}
    COMMENT "Generating atoms-extern.h"
    VERBATIM
)
add_custom_command(
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/build-utils/atoms-intern.sh
    ARGS ${ATOM_LIST_FILE} > ${BUILD_DIR}/atoms-intern.h
    OUTPUT ${BUILD_DIR}/atoms-intern.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${ATOM_LIST_FILE}
    COMMENT "Generating atoms-intern.h"
    VERBATIM
)
add_custom_target(generate_atoms
    DEPENDS ${BUILD_DIR}/atoms-extern.h ${BUILD_DIR}/atoms-intern.h
)
add_dependencies(${APP_NAME} generate_atoms)
