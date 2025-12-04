find_package(GTest CONFIG REQUIRED)

add_executable(libcuebin_tests
    testMsf.cpp
    testCueParser.cpp
    testDisc.cpp
)

target_link_libraries(libcuebin_tests
    PRIVATE
        libcuebin
        GTest::gtest
        GTest::gtest_main
)

target_compile_definitions(libcuebin_tests PRIVATE
    TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

include(GoogleTest)
gtest_discover_tests(libcuebin_tests)
