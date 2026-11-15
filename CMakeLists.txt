cmake_minimum_required(VERSION 3.20)
project(oxide LANGUAGES CXX)

option(OXIDE_BUILD_TESTS "Build sanitizer and regression tests" OFF)
option(OXIDE_BUILD_FUZZERS "Build libFuzzer targets (requires Clang)" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()

add_library(oxide STATIC
    src/oxide/io.cpp
    src/oxide/io/error.cpp
    src/oxide/log/logger.cpp
    src/oxide/panic.cpp
)

target_include_directories(oxide PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
# The selected standard also selects the formatting backend in public headers.
target_compile_features(oxide PUBLIC cxx_std_17 "cxx_std_${CMAKE_CXX_STANDARD}")
set_target_properties(oxide PROPERTIES CXX_EXTENSIONS OFF)

if(UNIX)
    target_sources(oxide PRIVATE
        src/oxide/posix/common/semaphore.cpp
        src/oxide/posix/pal/semaphore.cpp
        src/oxide/posix/shared_memory.cpp
    )

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_sources(oxide PRIVATE src/oxide/posix/linux/semaphore.cpp)
    endif()

    find_package(Threads REQUIRED)
    target_link_libraries(oxide PUBLIC Threads::Threads)
endif()

if(CMAKE_CXX_STANDARD LESS 20)
    find_package(fmt CONFIG QUIET)
    if(NOT fmt_FOUND)
        include(FetchContent)
        FetchContent_Declare(fmt
            GIT_REPOSITORY https://github.com/fmtlib/fmt.git
            GIT_TAG 11.2.0
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(fmt)
    endif()

    target_link_libraries(oxide PUBLIC fmt::fmt)
endif()

add_executable(slice_uppercase examples/slice_uppercase.cpp)
target_link_libraries(slice_uppercase PRIVATE oxide)

if(OXIDE_BUILD_TESTS)
    enable_testing()

    add_executable(oxide_string_san
        src/oxide/string.san.cpp
        src/oxide/string.fuzz.cpp
    )
    target_link_libraries(oxide_string_san PRIVATE oxide)
    target_compile_options(oxide_string_san PRIVATE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )
    target_link_options(oxide_string_san PRIVATE -fsanitize=address,undefined)
    add_test(NAME oxide_string COMMAND oxide_string_san)
    set_tests_properties(oxide_string PROPERTIES TIMEOUT 60)

    if(UNIX)
        find_package(GTest CONFIG REQUIRED)

        add_executable(oxide_shared_memory_test src/oxide/posix/shared_memory.test.cpp)
        target_link_libraries(oxide_shared_memory_test PRIVATE oxide GTest::gtest_main)
        add_test(NAME oxide_shared_memory COMMAND oxide_shared_memory_test)
        set_tests_properties(oxide_shared_memory PROPERTIES TIMEOUT 60)
    endif()
endif()

if(OXIDE_BUILD_FUZZERS)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "OXIDE_BUILD_FUZZERS requires Clang with libFuzzer support")
    endif()

    add_executable(oxide_string_fuzz src/oxide/string.fuzz.cpp)
    target_link_libraries(oxide_string_fuzz PRIVATE oxide)
    target_compile_options(oxide_string_fuzz PRIVATE
        -fsanitize=fuzzer,address,undefined
        -fno-omit-frame-pointer
    )
    target_link_options(oxide_string_fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
endif()
