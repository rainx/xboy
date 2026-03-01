cmake_minimum_required(VERSION 3.10)
project(xboy VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
include(GoogleTest)
find_package(GTest REQUIRED)
find_package(SDL2 REQUIRED)
enable_testing()

# Core emulator sources (no SDL / platform code)
file(GLOB CORE_SOURCES
    src/mmu/*.cpp
    src/mmu/cartridge/*.cpp
    src/cpu/*.cpp
    src/timer/*.cpp
    src/ppu/*.cpp
    src/input/*.cpp
    src/apu/*.cpp
    src/kaitai-struct-gen/*.cpp
)

# Platform sources (SDL2)
file(GLOB PLATFORM_SOURCES
    src/platform/*.cpp
)

add_executable(xboy src/main.cpp)
target_sources(xboy PRIVATE ${CORE_SOURCES} ${PLATFORM_SOURCES})
target_include_directories(xboy PRIVATE ./src)
target_include_directories(xboy PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(xboy PRIVATE ${SDL2_LIBRARIES})

# thirdparty deps
add_subdirectory(thirdparty/kaitai_struct_cpp_stl_runtime)
target_link_libraries(xboy PRIVATE kaitai_struct_cpp_stl_runtime)
target_include_directories(xboy PRIVATE ./thirdparty/kaitai_struct_cpp_stl_runtime)

# Tests (no SDL / platform code needed)
file(GLOB TEST_SOURCES
    test/mmu/*.test.cpp
    test/cpu/*.test.cpp
    test/kaitai-struct-gen/*.test.cpp
)

add_executable(xboy_test test/main.cpp)
target_sources(xboy_test PRIVATE ${CORE_SOURCES} ${TEST_SOURCES})
target_include_directories(xboy_test PRIVATE ./src)
target_include_directories(xboy_test PRIVATE ${GTEST_INCLUDE_DIR})
target_link_libraries(xboy_test PRIVATE ${GTEST_LIBRARIES})
target_link_libraries(xboy_test PRIVATE kaitai_struct_cpp_stl_runtime)
target_include_directories(xboy_test PRIVATE ./thirdparty/kaitai_struct_cpp_stl_runtime)

gtest_discover_tests(xboy_test)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
