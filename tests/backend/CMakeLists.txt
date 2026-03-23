set(OUTPUT_UTILS_TEST_APP_NAME "zdwm-output-utils-tests")

add_executable(${OUTPUT_UTILS_TEST_APP_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/output_utils_test.c
    ${SOURCE_DIR}/backend/output_utils.c
)

target_include_directories(${OUTPUT_UTILS_TEST_APP_NAME} SYSTEM
    PRIVATE ${INCLUDE_DIR}
)

target_include_directories(${OUTPUT_UTILS_TEST_APP_NAME}
    PRIVATE ${SOURCE_DIR}
    PRIVATE ${BUILD_DIR}
)

target_link_libraries(${OUTPUT_UTILS_TEST_APP_NAME}
    PRIVATE m
)

add_test(NAME ${OUTPUT_UTILS_TEST_APP_NAME}
    COMMAND $<TARGET_FILE:${OUTPUT_UTILS_TEST_APP_NAME}>
)
