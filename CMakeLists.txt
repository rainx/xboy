cmake_minimum_required(VERSION 3.0.0)
project(xboy VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
include(GoogleTest)
find_package(GTest REQUIRED)
enable_testing()

file(GLOB SOURCES
    src/mmu/*.cpp
    src/mmu/cartridge/*.cpp
    src/cpu/*.cpp
)

add_executable(xboy src/main.cpp)
target_sources(xboy PRIVATE ${SOURCES})
target_include_directories(xboy PRIVATE ./src)


file(GLOB TEST_SOURCES
    test/mmu/*.test.cpp
)

# add google test executable
add_executable(xboy_test test/main.cpp)
target_sources(xboy_test PRIVATE ${SOURCES} ${TEST_SOURCES})
target_include_directories(xboy_test PRIVATE ./src)
target_include_directories(xboy_test PRIVATE ${GTEST_INCLUDE_DIR})
target_link_libraries(xboy_test PRIVATE ${GTEST_LIBRARIES})

gtest_discover_tests(xboy_test)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
