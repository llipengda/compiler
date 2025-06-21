cmake_minimum_required(VERSION 3.10)

project(compiler LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_TLS_VERIFY OFF)

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

# ---- 静态库 compiler_lib ----
file(GLOB_RECURSE LIB_SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)
add_library(compiler_lib STATIC ${LIB_SOURCES})
target_include_directories(compiler_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_compile_definitions(compiler_lib PUBLIC
        # SEMA_PROD_USE_INITIALIZER_LIST
        # USE_STD_REGEX
        # DEBUG
)

# ---- 主程序 compiler ----
add_executable(compiler ${CMAKE_SOURCE_DIR}/main.cpp) # 如无 main.cpp，跳过
target_link_libraries(compiler PRIVATE compiler_lib)

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
target_link_libraries(compiler_tests PRIVATE gtest_main compiler_lib)
target_include_directories(compiler_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        $<TARGET_PROPERTY:gtest,INTERFACE_INCLUDE_DIRECTORIES>
)

include(GoogleTest)
gtest_discover_tests(compiler_tests)
