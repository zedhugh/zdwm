cmake_minimum_required(VERSION 3.24)

set(APP_NAME "zdwm")
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
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

add_executable(${APP_NAME} main.c utils.c buffer.c backtrace.c)

# use C23
set_property(TARGET ${APP_NAME} PROPERTY C_STANDARD 23)

find_package(PkgConfig REQUIRED)

pkg_check_modules(deps REQUIRED
    glib-2.0

    xcb
    xcb-aux
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

configure_file("${SOURCE_DIR}/config.h.in"
    "${BUILD_DIR}/config.h"
    ESCAPE_QUOTES
    @ONLY
)
