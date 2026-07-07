cmake_minimum_required(VERSION 3.15)
project(FogEngine VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enforce strict compiler rules to guarantee determinism
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

include_directories(include src)

# 1. FogEngine Shared Library (For Python ctypes)
add_library(fog_core SHARED 
    src/kernel/tables.cpp
    src/kernel/state_init.cpp
    src/kernel/threats.cpp
    src/kernel/move_gen_full.cpp
    src/kernel/visibility.cpp
    src/kernel/move_gen.cpp
    src/kernel/rules.cpp
    src/kernel/observation.cpp         
    src/runtime/scheduler.cpp
    src/sdk/bridge.cpp
    src/sdk/bridge_observations.cpp
)
target_compile_options(fog_core PRIVATE ${COMPILER_FLAGS})

# 2. Standalone C++ Executable
add_executable(fog_runner src/main.cpp)
target_link_libraries(fog_runner PRIVATE fog_core)
target_compile_options(fog_runner PRIVATE ${COMPILER_FLAGS})
# 3. GoogleTest Suite Integration
include(FetchContent)

# FIX 1: Suppress the FetchContent timestamp warning
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# FIX 2: Tell MinGW to ignore SSL verification for the GitHub download
set(CMAKE_TLS_VERIFY OFF)

# FIX 3: Force GoogleTest to build statically
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE) 

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(fog_tests 
    tests/test_determinism.cpp
    tests/test_kernel_core.cpp
    tests/test_rules.cpp
    tests/test_simd.cpp
    tests/test_end_to_end.cpp
)
target_link_libraries(fog_tests PRIVATE fog_core gtest_main)
target_compile_options(fog_tests PRIVATE ${COMPILER_FLAGS})

add_executable(fog_benchmark tests/benchmark_core.cpp)
target_link_libraries(fog_benchmark PRIVATE fog_core)
target_compile_options(fog_benchmark PRIVATE ${COMPILER_FLAGS})