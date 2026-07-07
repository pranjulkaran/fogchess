# @file CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(FogEngine VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings and optimizations (Linux only)

set(COMPILER_FLAGS
    -Wall
    -Wextra
    -Werror
    -O3
)

include(CheckCXXCompilerFlag)

check_cxx_compiler_flag("-mavx2" HAS_AVX2)

if(HAS_AVX2)
    list(APPEND COMPILER_FLAGS -mavx2)
endif()

# Include directories
include_directories(include src)

# 1. FogEngine Runtime & Kernel (Static Library)
# Must be SHARED for ctypes to load the file
add_library(fog_core SHARED 
    src/kernel/tables.cpp
    src/kernel/state_init.cpp
    src/kernel/threats.cpp
    src/kernel/move_gen_full.cpp
    src/kernel/visibility.cpp
    src/kernel/move_gen.cpp
    src/kernel/rules.cpp         
    src/runtime/scheduler.cpp
    src/sdk/bridge.cpp
)
target_compile_options(fog_core PRIVATE ${COMPILER_FLAGS})

# 2. The Main Executable Runner
add_executable(fog_runner src/main.cpp)
target_link_libraries(fog_runner PRIVATE fog_core)
target_compile_options(fog_runner PRIVATE ${COMPILER_FLAGS})

# 3. GoogleTest Suite (Downloads and builds GTest automatically)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(fog_tests 
    tests/test_simd.cpp
    tests/test_determinism.cpp
    tests/test_rules.cpp
    tests/test_kernel_core.cpp
)
target_link_libraries(fog_tests PRIVATE fog_core gtest_main)
target_compile_options(fog_tests PRIVATE ${COMPILER_FLAGS})