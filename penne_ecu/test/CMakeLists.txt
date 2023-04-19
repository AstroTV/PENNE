add_subdirectory(unity)

add_executable(
        tests
        tests.c
)

target_link_libraries(
        tests
        unity
)

target_include_directories(
        tests PUBLIC
        ${PROJECT_SOURCE_DIR}/test/unity
)

set_target_properties(
        tests
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${TEST_OUTPUT_PATH}"
)

add_test(tests "${TEST_OUTPUT_PATH}/tests")
