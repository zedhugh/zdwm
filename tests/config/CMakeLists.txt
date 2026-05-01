set(CONFIG_FIXTURE_TARGET "zdwm-config-fixture")
set(CONFIG_FIXTURE_NO_SETUP_TARGET "zdwm-config-fixture-no-setup")
set(CONFIG_LOADER_TEST_APP_NAME "zdwm-config-loader-tests")
set(RUNTIME_CONFIG_TEST_APP_NAME "zdwm-runtime-config-tests")

add_library(${CONFIG_FIXTURE_TARGET} SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/fixture_valid_config.c
)
target_include_directories(${CONFIG_FIXTURE_TARGET}
    PRIVATE ${INCLUDE_DIR}
)

add_library(${CONFIG_FIXTURE_NO_SETUP_TARGET} SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/fixture_without_setup.c
)

add_executable(${CONFIG_LOADER_TEST_APP_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/config_loader_test.c
    ${SOURCE_DIR}/base/log.c
    ${SOURCE_DIR}/config/loader.c
)
target_include_directories(${CONFIG_LOADER_TEST_APP_NAME} SYSTEM
    PRIVATE ${INCLUDE_DIR}
)
target_include_directories(${CONFIG_LOADER_TEST_APP_NAME}
    PRIVATE ${SOURCE_DIR}
    PRIVATE ${BUILD_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(${CONFIG_LOADER_TEST_APP_NAME}
    PRIVATE ${CMAKE_DL_LIBS}
)
add_dependencies(${CONFIG_LOADER_TEST_APP_NAME}
    ${CONFIG_FIXTURE_TARGET}
    ${CONFIG_FIXTURE_NO_SETUP_TARGET}
)

add_executable(${RUNTIME_CONFIG_TEST_APP_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/runtime_config_test.c
    ${SOURCE_DIR}/base/color.c
    ${SOURCE_DIR}/base/log.c
    ${SOURCE_DIR}/base/window_list.c
    ${SOURCE_DIR}/config/defaults.c
    ${SOURCE_DIR}/config/loader.c
    ${SOURCE_DIR}/config/runtime_config.c
    ${SOURCE_DIR}/core/action.c
    ${SOURCE_DIR}/core/binding.c
    ${SOURCE_DIR}/core/command_buffer.c
    ${SOURCE_DIR}/core/event.c
    ${SOURCE_DIR}/core/layer.c
    ${SOURCE_DIR}/core/layout.c
    ${SOURCE_DIR}/core/plan.c
    ${SOURCE_DIR}/core/policy.c
    ${SOURCE_DIR}/core/rules.c
    ${SOURCE_DIR}/core/runtime.c
    ${SOURCE_DIR}/core/state.c
    ${SOURCE_DIR}/core/window.c
    ${SOURCE_DIR}/layouts/fair.c
    ${SOURCE_DIR}/layouts/fullscreen.c
    ${SOURCE_DIR}/layouts/maximize.c
)
target_include_directories(${RUNTIME_CONFIG_TEST_APP_NAME} SYSTEM
    PRIVATE ${INCLUDE_DIR}
)
target_include_directories(${RUNTIME_CONFIG_TEST_APP_NAME}
    PRIVATE ${SOURCE_DIR}
    PRIVATE ${BUILD_DIR}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(${RUNTIME_CONFIG_TEST_APP_NAME}
    PRIVATE m
    PRIVATE ${deps_xkbcommon_MODULE_NAME}
    PRIVATE ${CMAKE_DL_LIBS}
)
add_dependencies(${RUNTIME_CONFIG_TEST_APP_NAME}
    ${CONFIG_FIXTURE_TARGET}
    ${CONFIG_FIXTURE_NO_SETUP_TARGET}
)

add_test(NAME ${CONFIG_LOADER_TEST_APP_NAME}
    COMMAND $<TARGET_FILE:${CONFIG_LOADER_TEST_APP_NAME}>
            $<TARGET_FILE:${CONFIG_FIXTURE_TARGET}>
            $<TARGET_FILE:${CONFIG_FIXTURE_NO_SETUP_TARGET}>
)

add_test(NAME ${RUNTIME_CONFIG_TEST_APP_NAME}
    COMMAND $<TARGET_FILE:${RUNTIME_CONFIG_TEST_APP_NAME}>
            $<TARGET_FILE:${CONFIG_FIXTURE_TARGET}>
            $<TARGET_FILE:${CONFIG_FIXTURE_NO_SETUP_TARGET}>
)
