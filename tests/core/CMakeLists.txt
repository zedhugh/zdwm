set(TEST_APP_NAME "zdwm-tests")

add_executable(${TEST_APP_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    ${SOURCE_DIR}/base/log.c
    ${SOURCE_DIR}/backend/output_utils.c
    ${SOURCE_DIR}/config/defaults.c
    ${SOURCE_DIR}/config/loader.c
    ${SOURCE_DIR}/config/runtime_config.c
    ${SOURCE_DIR}/backend/x11/backend.c
    ${SOURCE_DIR}/backend/x11/event.c
    ${SOURCE_DIR}/core/layout.c
    ${SOURCE_DIR}/core/runtime.c
    ${SOURCE_DIR}/core/state.c
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
