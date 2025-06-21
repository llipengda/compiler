cmake_minimum_required(VERSION 3.10)

project(compiler LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)

file(GLOB_RECURSE SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)

add_executable(compiler ${SOURCES})

target_compile_definitions(compiler PUBLIC
    # SEMA_PROD_USE_INITIALIZER_LIST # Use initializer lists when building sema_prod
    # USE_STD_REGEX                  # Use std::regex for regex operations
)

add_custom_command(
    TARGET compiler POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
)
