cmake_minimum_required(VERSION 3.30)
project(lfsr LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(prng
    src/lfsr/lfsr.cpp
)

target_include_directories(prng PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(prng PRIVATE -Wall -Wextra -Wpedantic)

add_executable(prng_app src/main.cpp)
target_link_libraries(prng_app PRIVATE prng)
