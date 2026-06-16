cmake_minimum_required(VERSION 3.20)
project(prng LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

add_library(lfsr
    src/lfsr/lfsr.cpp
)
target_include_directories(lfsr PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(lfsr PRIVATE -Wall -Wextra -Wpedantic)

add_executable(prng_app src/main.cpp)
target_link_libraries(prng_app PRIVATE lfsr)
target_compile_options(prng_app PRIVATE
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Release>:-O3 -march=native>
)
