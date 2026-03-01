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
    # Emscripten settings - output only JS and WASM, no HTML
    set(EMSCRIPTEN_FLAGS "-s USE_SDL=2 -s ALLOW_MEMORY_GROWTH=1")
    set(EMSCRIPTEN_FLAGS "${EMSCRIPTEN_FLAGS} -s EXPORTED_FUNCTIONS=_main,_malloc,_free")
    set(EMSCRIPTEN_FLAGS "${EMSCRIPTEN_FLAGS} -s EXPORTED_RUNTIME_METHODS=ccall,cwrap,FS")
    set(EMSCRIPTEN_FLAGS "${EMSCRIPTEN_FLAGS} -s FORCE_FILESYSTEM=1")
    set(EMSCRIPTEN_FLAGS "${EMSCRIPTEN_FLAGS} -s MODULARIZE=1 -s EXPORT_NAME='XBoyWASM'")
    set(EMSCRIPTEN_FLAGS "${EMSCRIPTEN_FLAGS} -s WASM=1")
    set(EMSCRIPTEN_FLAGS "${EMSCRIPTEN_FLAGS} --bind")
    
    # Enable exceptions for proper error handling
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
    set(EMSCRIPTEN_FLAGS "${EMSCRIPTEN_FLAGS} -fexceptions")
    
    # Output file name: xboy-wasm.js and xboy-wasm.wasm
    set(CMAKE_EXECUTABLE_SUFFIX "-wasm.js")
    
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EMSCRIPTEN_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${EMSCRIPTEN_FLAGS}")
    
    # Define for conditional compilation
    add_definitions(-DEMSCRIPTEN)
else()
    # Native configuration
    find_package(SDL2 REQUIRED)
    include(CTest)
    if(BUILD_TESTING)
        include(GoogleTest)
        find_package(GTest REQUIRED)
        enable_testing()
    endif()
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
    )
else()
    set(PLATFORM_SOURCES
        src/platform/sdl_display.cpp
        src/platform/sdl_audio.cpp
        src/platform/sdl_input.cpp
    )
endif()

# Main executable
if(EMSCRIPTEN)
    add_executable(xboy src/web_main.cpp)
else()
    add_executable(xboy src/main.cpp)
endif()
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

# Tests (only for native builds with testing enabled)
if(NOT EMSCRIPTEN AND BUILD_TESTING)
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