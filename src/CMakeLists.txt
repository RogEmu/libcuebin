find_package(spdlog CONFIG REQUIRED)

add_library(${PROJECT_NAME}
    msf.cpp
    cueTypes.cpp
    cueParser.cpp
    track.cpp
    disc.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE spdlog::spdlog
)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()


option(LIBCUEBIN_COVERAGE "Enable code coverage with gcov/gcovr" OFF)

if(LIBCUEBIN_COVERAGE)
    target_compile_options(${PROJECT_NAME} PRIVATE --coverage)
    target_link_libraries(${PROJECT_NAME} PRIVATE gcov)
endif()
