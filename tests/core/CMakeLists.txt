set(TEST_APP_NAME "zdwm-tests")

add_executable(${TEST_APP_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    ${SOURCE_DIR}/base/color.c
    ${SOURCE_DIR}/base/log.c
    ${SOURCE_DIR}/base/window_list.c
    ${SOURCE_DIR}/backend/output_utils.c
    ${SOURCE_DIR}/config/defaults.c
    ${SOURCE_DIR}/config/loader.c
    ${SOURCE_DIR}/config/runtime_config.c
    ${SOURCE_DIR}/backend/x11/backend.c
    ${SOURCE_DIR}/backend/x11/event.c
    ${SOURCE_DIR}/backend/x11/window.c
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

target_include_directories(${TEST_APP_NAME} SYSTEM
    PRIVATE ${deps_INCLUDE_DIRS}
    PRIVATE ${INCLUDE_DIR}
)
target_include_directories(${TEST_APP_NAME}
    PRIVATE ${SOURCE_DIR}
    PRIVATE ${BUILD_DIR}
)

target_link_libraries(${TEST_APP_NAME}
    PRIVATE m
    PRIVATE ${CMAKE_DL_LIBS}
    PRIVATE ${deps_LIBRARIES}
)

if(HAS_EXECINFO AND LIB_EXECINFO)
    target_link_libraries(${TEST_APP_NAME} PRIVATE ${LIB_EXECINFO})
endif()

add_test(NAME ${TEST_APP_NAME} COMMAND $<TARGET_FILE:${TEST_APP_NAME}>)

set_tests_properties(${TEST_APP_NAME} PROPERTIES
    ENVIRONMENT "DISPLAY=:3"
)
