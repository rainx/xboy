cmake_minimum_required(VERSION 3.10)
project(xboy VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Detect Emscripten
if(DEFINED ENV{EM_CONFIG})
    set(EMSCRIPTEN TRUE)
    message(STATUS "Emscripten detected, configuring for WebAssembly build")
endif()

# Platform-specific configuration
if(EMSCRIPTEN)
    # Emscripten settings
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_SDL=2")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s ALLOW_MEMORY_GROWTH=1")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s EXPORTED_FUNCTIONS=_main,_malloc,_free")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s EXPORTED_RUNTIME_METHODS=ccall,cwrap")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s MODULARIZE=1")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s EXPORT_NAME='XBoy'")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s WASM=1")
    
    # Define for conditional compilation
    add_definitions(-DEMSCRIPTEN)
else()
    # Native configuration
    include(CTest)
    include(GoogleTest)
    find_package(GTest REQUIRED)
    find_package(SDL2 REQUIRED)
    enable_testing()
endif()

# Core emulator sources (no SDL / platform code)
file(GLOB CORE_SOURCES
    src/mmu/*.cpp
    src/mmu/cartridge/*.cpp
    src/cpu/*.cpp
    src/timer/*.cpp
    src/ppu/*.cpp
    src/input/*.cpp
    src/apu/*.cpp
    src/test_runner/*.cpp
    src/state/*.cpp
    src/debugger/*.cpp
    src/kaitai-struct-gen/*.cpp
)

# Platform-specific sources
if(EMSCRIPTEN)
    set(PLATFORM_SOURCES
        src/platform/web_display.cpp
        src/platform/web_audio.cpp
        src/platform/web_input.cpp
        src/platform/web_filesystem.cpp
        bindings.cpp
    )
else()
    set(PLATFORM_SOURCES
        src/platform/sdl_display.cpp
        src/platform/sdl_audio.cpp
        src/platform/sdl_input.cpp
    )
endif()

# Main executable
add_executable(xboy src/main.cpp)
target_sources(xboy PRIVATE ${CORE_SOURCES} ${PLATFORM_SOURCES})
target_include_directories(xboy PRIVATE ./src)

# Platform-specific linking
if(NOT EMSCRIPTEN)
    target_include_directories(xboy PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(xboy PRIVATE ${SDL2_LIBRARIES})
endif()

# Thirdparty dependencies
add_subdirectory(thirdparty/kaitai_struct_cpp_stl_runtime)
target_link_libraries(xboy PRIVATE kaitai_struct_cpp_stl_runtime)
target_include_directories(xboy PRIVATE ./thirdparty/kaitai_struct_cpp_stl_runtime)

# Tests (only for native builds)
if(NOT EMSCRIPTEN)
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
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)