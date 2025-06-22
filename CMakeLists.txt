cmake_minimum_required(VERSION 3.10)

project(compiler LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_TLS_VERIFY OFF)

option(CODE_COVERAGE "Enable coverage reporting" OFF)
option(SEMA_PROD_USE_INITIALIZER_LIST "Use initializer list for sema production" OFF)
option(USE_STD_REGEX "Use std::regex for lexer" OFF)
option(DEBUG "Enable debug mode" OFF)

if(CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(STATUS "Code coverage enabled")
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)

    if(NOT DEBUG)
        set(DEBUG ON CACHE BOOL "Enable debug mode" FORCE)
        message(STATUS "DEBUG automatically enabled because CODE_COVERAGE is ON")
    endif()
endif()

# ---- Fetch GoogleTest ----
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/releases/download/v1.17.0/googletest-1.17.0.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP true
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ---- Include path ----
include_directories(${CMAKE_SOURCE_DIR}/include)

# ---- 静态库 compiler ----
file(GLOB_RECURSE LIB_SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)
add_library(compiler STATIC ${LIB_SOURCES})
target_include_directories(compiler PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_compile_definitions(compiler PUBLIC
    $<$<BOOL:${SEMA_PROD_USE_INITIALIZER_LIST}>:SEMA_PROD_USE_INITIALIZER_LIST>
    $<$<BOOL:${USE_STD_REGEX}>:USE_STD_REGEX>
    $<$<BOOL:${DEBUG}>:DEBUG>
)

# ---- simple_cc ----
file(GLOB_RECURSE SIMPLE_CC_SOURCES ${CMAKE_SOURCE_DIR}/simple_cc/*.cpp)
add_executable(simple_cc ${SIMPLE_CC_SOURCES})
target_include_directories(simple_cc PRIVATE
        ${CMAKE_SOURCE_DIR}/simple_cc/include
        $<TARGET_PROPERTY:compiler,INTERFACE_INCLUDE_DIRECTORIES>
)
target_link_libraries(simple_cc PRIVATE compiler)

add_custom_command(
        TARGET compiler POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
)

# ---- 单元测试 ----
enable_testing()
file(GLOB_RECURSE TEST_SOURCES ${CMAKE_SOURCE_DIR}/tests/*.cpp)

add_executable(compiler_tests ${TEST_SOURCES})
target_link_libraries(compiler_tests PRIVATE gtest_main compiler)
target_include_directories(compiler_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        $<TARGET_PROPERTY:gtest,INTERFACE_INCLUDE_DIRECTORIES>
)

include(GoogleTest)
gtest_discover_tests(compiler_tests)
