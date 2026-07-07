cmake_minimum_required(VERSION 3.15)

project(FogEngine VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CheckCXXCompilerFlag)

#
# Compiler flags
#

set(COMMON_FLAGS "")

if(MSVC)
    list(APPEND COMMON_FLAGS
        /W4
        /WX
    )
else()
    list(APPEND COMMON_FLAGS
        -Wall
        -Wextra
        -Werror
        -O3
    )
endif()

#
# Enable AVX2 only on x86/x64
#

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i686|x86")

    if(MSVC)

        list(APPEND COMMON_FLAGS /arch:AVX2)

    else()

        check_cxx_compiler_flag("-mavx2" HAS_AVX2)

        if(HAS_AVX2)
            list(APPEND COMMON_FLAGS -mavx2)
        endif()

    endif()

endif()

#
# Fog Core Library
#

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

target_include_directories(fog_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src
)

target_compile_options(fog_core PRIVATE ${COMMON_FLAGS})

#
# Runner
#

add_executable(fog_runner
    src/main.cpp
)

target_link_libraries(fog_runner PRIVATE fog_core)
target_compile_options(fog_runner PRIVATE ${COMMON_FLAGS})

#
# GoogleTest
#

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(fog_tests
    tests/test_simd.cpp
    tests/test_determinism.cpp
    tests/test_rules.cpp
    tests/test_kernel_core.cpp
)

target_link_libraries(fog_tests
    PRIVATE
        fog_core
        gtest_main
)

target_compile_options(fog_tests PRIVATE ${COMMON_FLAGS})

include(GoogleTest)

gtest_discover_tests(fog_tests)