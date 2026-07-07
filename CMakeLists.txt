cmake_minimum_required(VERSION 3.20)
project(FogEngine VERSION 1.0.0 LANGUAGES CXX C)

==============================================================================

GLOBAL C++ CONFIGURATION (Data-Oriented, High-Performance)

==============================================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Strict ISO C++

Enforce build types (Default to Release if not specified)

if(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

==============================================================================

STRICT COMPILER FLAGS (Day One Requirement: No Warnings)

==============================================================================

set(FOG_STRICT_FLAGS "")
if(MSVC)
# Windows/MSVC: Level 4 warnings, treat warnings as errors, fast floating point
list(APPEND FOG_STRICT_FLAGS /W4 /WX /fp:fast /wd4201)
else()
# Linux/macOS (GCC/Clang): All extra warnings, treat warnings as errors, AVX2 SIMD
list(APPEND FOG_STRICT_FLAGS -Wall -Wextra -Werror -mavx2 -mfma)
endif()

==============================================================================

MODULE 1: THE KERNEL (Pure Math, No Allocations, No Dependencies)

==============================================================================

add_library(fog_kernel STATIC
src/kernel/simulator.cpp
src/kernel/move_gen.cpp
src/kernel/visibility.cpp
)
target_include_directories(fog_kernel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(fog_kernel PRIVATE ${FOG_STRICT_FLAGS})

==============================================================================

MODULE 2: THE RUNTIME (Thread Pools, Arena Allocators, Job Queues)

==============================================================================

add_library(fog_runtime STATIC
src/runtime/arena_allocator.cpp
src/runtime/job_system.cpp
src/runtime/ring_buffer.cpp
)
target_include_directories(fog_runtime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(fog_runtime PRIVATE fog_kernel)

Add threading support for the runtime layer

find_package(Threads REQUIRED)
target_link_libraries(fog_runtime PRIVATE Threads::Threads)
target_compile_options(fog_runtime PRIVATE ${FOG_STRICT_FLAGS})

==============================================================================

MODULE 3: THE C-ABI SDK (The Bridge for Python/C#/Unity)

==============================================================================

add_library(fog_sdk SHARED
src/sdk/c_api_bridge.cpp
)
target_include_directories(fog_sdk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(fog_sdk PRIVATE fog_runtime)
target_compile_options(fog_sdk PRIVATE ${FOG_STRICT_FLAGS})

==============================================================================

TESTING INFRASTRUCTURE (GoogleTest Integration)

==============================================================================

enable_testing()
include(FetchContent)
FetchContent_Declare(
googletest
GIT_REPOSITORY https://github.com/google/googletest.git
GIT_TAG        v1.14.0
)

Make gtest silent to avoid cluttering our build output

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

Engine Determinism & Bitboard Tests

add_executable(fog_tests
tests/test_determinism.cpp
tests/test_move_gen.cpp
tests/test_memory_arena.cpp
)
target_link_libraries(fog_tests PRIVATE fog_sdk GTest::gtest_main)
target_compile_options(fog_tests PRIVATE ${FOG_STRICT_FLAGS})

Register tests with CTest

include(GoogleTest)
gtest_discover_tests(fog_tests)