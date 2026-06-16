cmake_minimum_required(VERSION 3.20)
project(prng LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

include(FetchContent)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.15.3
)
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(SPDLOG_COMPILED_LIB ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(spdlog)

add_library(lfsr
    src/lfsr/lfsr.cpp
    src/grendizer/grendizer.cpp
)
target_include_directories(lfsr PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(lfsr PRIVATE -Wall -Wextra -Wpedantic)

add_executable(prng_app src/main.cpp)
target_link_libraries(prng_app PRIVATE lfsr spdlog::spdlog)

target_compile_options(prng_app PRIVATE
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Release>:-O3 -march=native>
)
